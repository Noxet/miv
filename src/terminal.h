#pragma once

int termSetupSignals();
int termEnableRawMode();
int termDisableRawMode();
char termReadKey();
static void sigHandler(int sig);
