#pragma once

struct ConsoleOptions {
    bool noConfirm;
    bool directDelete;
};

int RunConsole(const ConsoleOptions& options);