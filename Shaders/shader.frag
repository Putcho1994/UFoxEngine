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

vec3 lerp(vec3 colorA, vec3 colorB, float value)
{
    return (colorA + value*(colorB-colorA));
}

void main() {
    // Calculate pixel position based on texture coordinates and scale
    vec2 pixelPos = fragTexCoord * fragScale;
    vec2 center = fragScale * 0.5;
    vec2 rectCorner = fragScale * 0.5;
    vec2 centerPos = pixelPos - center;

    // Compute border corner
    vec2 borderCorner = rectCorner;
    if (pixelPos.y >= center.y) {
        borderCorner.y -= params.borderThickness.z; // Bottom thickness
    } else {
        borderCorner.y -= params.borderThickness.x; // Top thickness
    }
    if (pixelPos.x >= center.x) {
        borderCorner.x -= params.borderThickness.y; // Right thickness
    } else {
        borderCorner.x -= params.borderThickness.w; // Left thickness
    }

    // Compute SDF distances
    float shapeDistance = roundedBoxSDF(centerPos, rectCorner, params.cornerRadius);
    float backgroundDistance = roundedBoxSDF(centerPos, borderCorner, params.cornerRadius);

    // Determine the closest edge color for the entire shape
    vec4 borderColor = black;
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
    } else {
        borderColor = params.borderLeftColor;
    }

    //Create the mask using shapeDistance and backgroundDistance transitions
    vec4 marginMask = white * smoothstep(-1.5, -0.0, backgroundDistance);
    vec4 backgroundMask = white * smoothstep(-1.0, 0.5, shapeDistance);
    //combine Fragcolor with textColor and borderColor
    vec4 texColor = texture(texSampler, fragTexCoord);
    //lerp between black and white color with vertex color alpha or other source
    vec4 mainColorOpacity = vec4(lerp(black.rgb, white.rgb, fragColor.a), 1);
    //prepare mask
    float mainMask = mix(mainColorOpacity,black,backgroundMask).r;
    float borderMask = mix(black,white,marginMask).r;
    float invertMainMask = mix(white,black,backgroundMask).r;
    float finalBorderMask = borderMask * invertMainMask;
    //coloring
    vec4 finalBorderColor =  borderMask * borderColor;
    vec4 finalMainColor = mainMask * texColor;
    vec4 finalColor = mix(finalMainColor, finalBorderColor, finalBorderMask);

    outColor = finalColor;
}