#pragma once


void logInit();
void logGeneric(const char* format, ...);
void logError(int ec, const char* format, ...);
