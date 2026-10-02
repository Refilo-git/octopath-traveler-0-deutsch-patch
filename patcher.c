#include <windows.h>
#include <commdlg.h>
#include <stdio.h>
#include <string.h>
typedef struct { unsigned long long off; int len; const unsigned char *oldb,*newb; const char *name; const unsigned char *altb; } Patch;
#include "patches.h"
#define NP (int)(sizeof(PATCHES)/sizeof(PATCHES[0]))
#define EXE_REL "steamapps\\common\\Octopath_Traveler0\\Octopath_Traveler0\\Binaries\\Win64\\Octopath_Traveler0-Win64-Shipping.exe"
static int isfile(const char*p){DWORD a=GetFileAttributesA(p);return a!=INVALID_FILE_ATTRIBUTES&&!(a&FILE_ATTRIBUTE_DIRECTORY);}
static int try_lib(const char*lib,char*out){ /* lib ohne abschliessenden Backslash */
  char t[MAX_PATH*2]; snprintf(t,sizeof t,"%s\\%s",lib,EXE_REL); if(isfile(t)){strcpy(out,t);return 1;} return 0;}
static int steam_root(char*out){
  const char*keys[][2]={{"Software\\Valve\\Steam","SteamPath"},{"SOFTWARE\\WOW6432Node\\Valve\\Steam","InstallPath"}};
  HKEY roots[2]={HKEY_CURRENT_USER,HKEY_LOCAL_MACHINE};
  for(int i=0;i<2;i++){DWORD n=MAX_PATH*2; out[0]=0;
    if(RegGetValueA(roots[i],keys[i][0],keys[i][1],RRF_RT_REG_SZ,NULL,out,&n)==ERROR_SUCCESS&&out[0]){
      for(char*q=out;*q;q++) if(*q=='/')*q='\\'; return 1;}}
  return 0;}
static int find_exe(char*out){
  char root[MAX_PATH*2];
  if(steam_root(root)){
    if(try_lib(root,out))return 1;
    char vdf[MAX_PATH*2]; snprintf(vdf,sizeof vdf,"%s\\steamapps\\libraryfolders.vdf",root);
    FILE*f=fopen(vdf,"r"); if(f){char line[2048];
      while(fgets(line,sizeof line,f)){char*p=strstr(line,"\"path\""); if(!p)continue;
        p=strchr(p+6,'"'); if(!p)continue; p++; char lib[MAX_PATH*2]; int k=0;
        while(*p&&*p!='"'&&k<(int)sizeof lib-1){ if(*p=='\\'&&p[1]=='\\')p++; lib[k++]=*p++; } lib[k]=0;
        if(try_lib(lib,out)){fclose(f);return 1;}}
      fclose(f);}
  }
  const char*dl[]={"C:\\Program Files (x86)\\Steam","C:\\Program Files\\Steam","D:\\SteamLibrary","E:\\SteamLibrary","D:\\Steam","E:\\Steam","F:\\SteamLibrary"};
  for(int i=0;i<(int)(sizeof dl/sizeof*dl);i++) if(try_lib(dl[i],out))return 1;
  return 0;}
static void msg(const char*t,UINT f){MessageBoxA(NULL,t,"Octopath Traveler 0 - Deutsch-Patch",f|MB_SETFOREGROUND);}
static int rd(FILE*f,unsigned long long off,unsigned char*b,int n){
  if(_fseeki64(f,(long long)off,SEEK_SET))return 0; return fread(b,1,n,f)==(size_t)n;}
static int wr(FILE*f,unsigned long long off,const unsigned char*b,int n){
  if(_fseeki64(f,(long long)off,SEEK_SET))return 0; return fwrite(b,1,n,f)==(size_t)n;}
int WINAPI WinMain(HINSTANCE h,HINSTANCE p,LPSTR c,int s){
  char path[MAX_PATH*2]; path[0]=0; int ok=0;
  if(find_exe(path)){
    char q[MAX_PATH*2+200]; snprintf(q,sizeof q,"Octopath Traveler 0 wurde hier gefunden:\n\n%s\n\nIst das die richtige Installation, die gepatcht werden soll?\n(Nein = Datei selbst auswaehlen)",path);
    int r=MessageBoxA(NULL,q,"Octopath Traveler 0 - Deutsch-Patch",MB_YESNOCANCEL|MB_ICONQUESTION|MB_SETFOREGROUND);
    if(r==IDCANCEL)return 0; ok=(r==IDYES);
  }
  if(!ok){
    msg("Bitte im naechsten Fenster\nOctopath_Traveler0-Win64-Shipping.exe auswaehlen\n(Ordner: ...\\Octopath_Traveler0\\Octopath_Traveler0\\Binaries\\Win64).",MB_ICONINFORMATION);
    OPENFILENAMEA o; memset(&o,0,sizeof o); path[0]=0; o.lStructSize=sizeof o; o.lpstrFile=path; o.nMaxFile=sizeof path;
    o.lpstrFilter="Spiel-exe\0Octopath_Traveler0-Win64-Shipping.exe\0Alle\0*.*\0"; o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
    if(!GetOpenFileNameA(&o))return 0;
  }
  FILE*f=fopen(path,"rb"); if(!f){msg("Datei kann nicht geoeffnet werden (laeuft das Spiel noch?).",MB_ICONERROR);return 1;}
  int nOld=0,nNew=0,nBad=0,nAlt=0;
  for(int i=0;i<NP;i++){unsigned char b[512]; if(!rd(f,PATCHES[i].off,b,PATCHES[i].len)){nBad++;continue;}
    if(!memcmp(b,PATCHES[i].oldb,PATCHES[i].len))nOld++; else if(!memcmp(b,PATCHES[i].newb,PATCHES[i].len))nNew++; else if(PATCHES[i].altb&&!memcmp(b,PATCHES[i].altb,PATCHES[i].len))nAlt++; else nBad++;}
  fclose(f);
  char buf[1024]; int upg=(nAlt>0);
  if(nNew==NP){
    if(MessageBoxA(NULL,"Der Patch ist bereits angewendet.\n\nSoll er rueckgaengig gemacht werden (Original wiederherstellen)?","Octopath Traveler 0 - Deutsch-Patch",MB_YESNO|MB_ICONQUESTION)!=IDYES)return 0;
    f=fopen(path,"r+b"); if(!f){msg("Datei kann nicht beschrieben werden (Spiel beendet? Als Administrator starten?).",MB_ICONERROR);return 1;}
    for(int i=0;i<NP;i++) if(!wr(f,PATCHES[i].off,PATCHES[i].oldb,PATCHES[i].len)){fclose(f);msg("Schreibfehler beim Rueckgaengigmachen.",MB_ICONERROR);return 1;}
    fclose(f); msg("Original wiederhergestellt.",MB_ICONINFORMATION); return 0;
  }
  if(nBad||(nOld!=NP&&!nAlt)){
    sprintf(buf,"Diese Spiel-exe passt nicht zum Patch (%d von %d Stellen stimmen nicht).\nVermutlich wurde das Spiel aktualisiert. Es wurde NICHTS veraendert.",nBad+nNew,NP);
    msg(buf,MB_ICONERROR); return 1;}
  if(MessageBoxA(NULL,upg?"Eine aeltere Version des Patches wurde erkannt.\nSie wird jetzt auf die neue Version aktualisiert (zusaetzlich: deutsche Texturen).\nDie vorhandene Sicherung (.original) bleibt unveraendert.\n\nFortfahren?":"Die Spiel-exe wird jetzt gepatcht (Deutsch als 6. Textsprache).\nVorher wird eine Sicherung angelegt:\nOctopath_Traveler0-Win64-Shipping.exe.original\n\nFortfahren?","Octopath Traveler 0 - Deutsch-Patch",MB_YESNO|MB_ICONQUESTION)!=IDYES)return 0;
  char bak[MAX_PATH*2+16]; sprintf(bak,"%s.original",path);
  if(GetFileAttributesA(bak)==INVALID_FILE_ATTRIBUTES){ if(!CopyFileA(path,bak,TRUE)){msg("Sicherung konnte nicht angelegt werden. Abbruch, nichts veraendert.",MB_ICONERROR);return 1;} }
  f=fopen(path,"r+b"); if(!f){msg("Datei kann nicht beschrieben werden (Spiel beendet? Als Administrator starten?).",MB_ICONERROR);return 1;}
  for(int i=0;i<NP;i++) if(!wr(f,PATCHES[i].off,PATCHES[i].newb,PATCHES[i].len)){fclose(f);
    msg("Schreibfehler. Bitte die Sicherung (.original) zurueckkopieren.",MB_ICONERROR);return 1;}
  fclose(f);
  { char me[MAX_PATH*2],src[MAX_PATH*2],dst[MAX_PATH*2],*sl; GetModuleFileNameA(NULL,me,sizeof me);
    sl=strrchr(me,'\\'); if(sl)*sl=0; snprintf(src,sizeof src,"%s\\Octopath_TravelerDE_P.pak",me);
    strcpy(dst,path); for(int k=0;k<3;k++){sl=strrchr(dst,'\\'); if(sl)*sl=0;} /* ...\\Octopath_Traveler0\\Octopath_Traveler0 */
    strcat(dst,"\\Content\\Paks\\Octopath_TravelerDE_P.pak");
    if(isfile(src)){
      char q[MAX_PATH*2+200]; snprintf(q,sizeof q,"Fertig. Die Spiel-exe ist gepatcht.\n\nSoll die Datei Octopath_TravelerDE_P.pak jetzt auch in den Paks-Ordner kopiert werden?\n\n%s",dst);
      if(MessageBoxA(NULL,q,"Octopath Traveler 0 - Deutsch-Patch",MB_YESNO|MB_ICONQUESTION)==IDYES){
        if(CopyFileA(src,dst,FALSE)) msg("Pak kopiert. Alles fertig - Spiel starten und unter Optionen Deutsch waehlen.",MB_ICONINFORMATION);
        else msg("Pak konnte nicht kopiert werden. Bitte Octopath_TravelerDE_P.pak von Hand in den Ordner Content\\Paks kopieren.",MB_ICONWARNING);
      }
    } else msg("Fertig. Die Spiel-exe ist gepatcht.\n\nDie Datei Octopath_TravelerDE_P.pak muss noch in den Ordner Content\\Paks kopiert werden.",MB_ICONINFORMATION);
  }
  return 0;
}
