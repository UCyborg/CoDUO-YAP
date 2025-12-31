#pragma once
#include <safetyhook.hpp>
#include "MemoryMgr.h"
template<typename T, typename Fn>
SafetyHookInline* CreateInlineHook(T target, Fn destination, SafetyHookInline::Flags flags = SafetyHookInline::Default);


template<typename T>
SafetyHookMid* CreateMidHook(T target, safetyhook::MidHookFn destination, safetyhook::MidHook::Flags flags = safetyhook::MidHook::Default);