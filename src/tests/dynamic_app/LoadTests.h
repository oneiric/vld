#pragma once

HMODULE LoadDynamicTests();
void RunLoaderTests(HMODULE hdyn, bool resolve);
HMODULE LoadMFCTests();
void RunMFCLoaderTests(HMODULE hmfcLib, bool resolve);
