#version 450

layout(location = 0) in vec4 fragColor; // RGBA, with alpha as a base value
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec2 fragScale; // Rectangle size from vertex shader

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform RoundedRectParams {
    vec4 cornerRadius;       // x: top-left, y: top-right, z: bottom-left, w: bottom-right
    vec4 borderThickness;   // x: top, y: right, z: bottom, w: left
    vec4 borderTopColor;    // Color for top border
    vec4 borderRightColor;  // Color for right border
    vec4 borderBottomColor; // Color for bottom border
    vec4 borderLeftColor;   // Color for left border
} params;

layout(location = 0) out vec4 outColor;

const vec4 black = vec4(0.0, 0.0, 0.0, 1.0);
const vec4 white = vec4(1.0, 1.0, 1.0, 1.0);

// SDF for a rounded rectangle with per-corner radius
float roundedBoxSDF(vec2 centerPosition, vec2 size, vec4 radius) {
    float finalRadius = (centerPosition.x < 0.0 && centerPosition.y > 0.0) ? radius.x : // Top-left
    (centerPosition.x > 0.0 && centerPosition.y > 0.0) ? radius.y : // Top-right
    (centerPosition.x < 0.0 && centerPosition.y < 0.0) ? radius.z : // Bottom-left
    radius.w; // Bottom-right
    vec2 q = abs(centerPosition) - size + finalRadius;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - finalRadius;
}

// Linear interpolation for vec3
vec3 lerp(vec3 colorA, vec3 colorB, float value) {
    return colorA + value * (colorB - colorA);
}

void main() {
    // Calculate pixel position and center
    vec2 pixelPos = fragTexCoord * fragScale;
    vec2 center = fragScale * 0.5;
    vec2 rectCorner = fragScale * 0.5;
    vec2 centerPos = pixelPos - center;

    // Compute border corner with optimized quadrant check
    vec2 borderCorner = rectCorner;
    float yQuad = step(center.y, pixelPos.y); // 0 if above center, 1 if below
    float xQuad = step(center.x, pixelPos.x); // 0 if left of center, 1 if right
    borderCorner.y -= mix(params.borderThickness.x, params.borderThickness.z, yQuad); // Top or bottom thickness
    borderCorner.x -= mix(params.borderThickness.w, params.borderThickness.y, xQuad); // Left or right thickness

    // Adjust inner corner radius based on border thickness
    vec4 adjustedCornerRadius = params.cornerRadius;
    float maxThicknessX = mix(params.borderThickness.w, params.borderThickness.y, xQuad); // Left or right max
    float maxThicknessY = mix(params.borderThickness.x, params.borderThickness.z, yQuad); // Top or bottom max
    adjustedCornerRadius.x = max(params.cornerRadius.x - (maxThicknessX + maxThicknessY) * 0.5, 0.0); // Top-left
    adjustedCornerRadius.y = max(params.cornerRadius.y - (maxThicknessX + maxThicknessY) * 0.5, 0.0); // Top-right
    adjustedCornerRadius.z = max(params.cornerRadius.z - (maxThicknessX + maxThicknessY) * 0.5, 0.0); // Bottom-left
    adjustedCornerRadius.w = max(params.cornerRadius.w - (maxThicknessX + maxThicknessY) * 0.5, 0.0); // Bottom-right

    // Compute SDF distances
    float shapeDistance = roundedBoxSDF(centerPos, rectCorner, params.cornerRadius);
    float backgroundDistance = roundedBoxSDF(centerPos, borderCorner, adjustedCornerRadius);

    // Determine the closest edge color for the entire shape
    vec4 borderColor = params.borderTopColor; // Default to top color as fallback
    float topDist = abs(pixelPos.y - (center.y + rectCorner.y));
    float rightDist = abs(pixelPos.x - (center.x + rectCorner.x));
    float bottomDist = abs(pixelPos.y - (center.y - rectCorner.y));
    float leftDist = abs(pixelPos.x - (center.x - rectCorner.x));
    float minDist = min(min(topDist, rightDist), min(bottomDist, leftDist));
    if (minDist == topDist) {
        borderColor = params.borderTopColor;
    } else if (minDist == rightDist) {
        borderColor = params.borderRightColor;
    } else if (minDist == bottomDist) {
        borderColor = params.borderBottomColor;
    } else if (minDist == leftDist) {
        borderColor = params.borderLeftColor;
    }

    // Create the mask using shapeDistance and backgroundDistance transitions
    vec4 marginMask = white * smoothstep(-0.8, 0.0, backgroundDistance);
    vec4 backgroundMask = white * smoothstep(-0.8, 0.8, shapeDistance);

    // Combine Fragcolor with textColor
    vec4 texColor = texture(texSampler, fragTexCoord);
    vec4 mainColorOpacity = vec4(lerp(black.rgb, white.rgb, fragColor.a), 1.0);
    float mainMask = mix(mainColorOpacity, black, backgroundMask).r;
    float borderMask = mix(black, white, marginMask).r;
    float invertMainMask = mix(white, black, backgroundMask).r;
    float finalBorderMask = borderMask * invertMainMask;

    // Coloring
    vec4 finalBorderColor = borderMask * borderColor;
    vec4 finalMainColor = mainMask * texColor;
    vec4 finalColor = mix(finalMainColor, finalBorderColor, finalBorderMask);

    outColor = finalColor;
}