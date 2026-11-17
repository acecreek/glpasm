#pragma once
#include "../AppInit/AppInit.h"

class CGLSufApp : public CWinApp
{
public:
    BOOL InitInstance() override;
    const AppConfig& GetConfig() const { return m_config; }

private:
    AppConfig m_config;
};

extern CGLSufApp theApp;

