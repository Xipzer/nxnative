// Weak defaults of functions hle_nvn.c defines, for the graphics tests that link nvn_mtl.m without it. They live apart from nvn_mtl.m because tests that #include both
// nvn_mtl.m and hle_nvn.c in one translation unit cannot hold a weak and a strong definition of the same function there
__attribute__((weak)) int nvn_var_fps(void) { return 0; }   // the cap of a variable frame step while it holds (0 = none)
