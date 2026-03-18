#pragma once

#include "../common.hpp"

#define VULKAN_HPP_NO_SMART_HANDLE 1
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS 1

#include <vulkan/vulkan.hpp>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan_structs.hpp>
#include <vk_mem_alloc.h>