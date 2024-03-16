#pragma once

int termSetupSignals();
int termEnableRawMode();
int termDisableRawMode();
int termGetWindowSize(int *rows, int *cols);
char termReadKey();
