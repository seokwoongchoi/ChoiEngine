#pragma once


const char* Application_GetName();
void Application_Initialize();
void Application_Finalize();
void Application_Frame(bool* show);


void SaveAllNodes();
void LoadAllNodes();
