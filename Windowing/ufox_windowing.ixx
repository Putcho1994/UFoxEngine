//
// Created by b-boy on 13.04.2025.
//
module;

#include <memory>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>


export module ufox_windowing;
       // import declaration

import fmt;
import vulkan_hpp;

export namespace ufox::windowing::SDL
{

       // Custom deleter for SDL_Window

       struct SDLWindowDeleter {
              void operator()(SDL_Window* window) const {
                     if (window) {
                            SDL_DestroyWindow(window);
                     }
              }
       };

       // Type alias for unique_ptr with SDL_Window and SDLWindowDeleter
       using Window = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

       class SDLException : public std::runtime_error {
       public:
              explicit SDLException(const std::string& msg) : std::runtime_error(msg + ": " + SDL_GetError()) {}
       };


       Window CreateWindow(const char* title, Uint32 flags) {
              vk::Bool32 hhh = true;

              if (!SDL_Init(SDL_INIT_VIDEO)) throw SDLException("Failed to initialize SDL");
              if (!SDL_Vulkan_LoadLibrary(nullptr)) throw SDLException("Failed to load Vulkan library");

              SDL_DisplayID primary = SDL_GetPrimaryDisplay();
              SDL_Rect usableBounds{};
              SDL_GetDisplayUsableBounds(primary, &usableBounds);

#ifdef UFOX_DEBUG
              fmt::println("Display usable bounds: {}x{}", usableBounds.w, usableBounds.h);
#endif

              Window window {nullptr, SDLWindowDeleter()};

              window.reset(SDL_CreateWindow(
                  title,
                  usableBounds.w - 4, usableBounds.h - 34, // Account for title bar and resize handle
                  flags));
              if (!window) throw SDLException("Failed to create window");

              SDL_SetWindowPosition(window.get(), 2, 32);

#ifdef UFOX_DEBUG
              fmt::println("Window : name {} : successfully created", title);
#endif



              return window;
       }





}