
#ifndef __S_MUSIC__
#define __S_MUSIC__

#include "zstring.h"
#include "tarray.h"
#include "name.h"
#include <zmusic.h>

class FileReader;

struct MusicCallbacks
{
	FString(*LookupFileName)(const char* fn, int &order);
	FileReader(*OpenMusic)(const char* fn);
};
void S_SetMusicCallbacks(MusicCallbacks* cb);

void S_CreateStream();
void S_PauseStream(bool pause);
void S_StopStream();
void S_SetStreamVolume(float vol);


//
void S_InitMusic ();
void S_ResetMusic ();


// Start music using <music_name>
bool S_StartMusic (const char *music_name);

// Start music using <music_name>, and set whether looping
bool S_ChangeMusic (const char *music_name, int order=0, bool looping=true, bool force=false);

void S_RestartMusic ();
void S_MIDIDeviceChanged(int newdev);

int S_GetMusic (const char **name);

// Stops the music for sure.
void S_StopMusic (bool force);

// Stop and resume music, during game PAUSE.
void S_PauseMusic ();
void S_ResumeMusic ();

//
// Updates music & sounds
//
void S_UpdateMusic ();

struct MidiDeviceSetting
{
	int device;
	FString args;
};

typedef TMap<FName, MidiDeviceSetting> MidiDeviceMap;
typedef TMap<FName, float> MusicVolumeMap;

extern MidiDeviceMap MidiDevices;
extern MusicVolumeMap MusicVolumes;

struct MusPlayingInfo
{
	FString name;
	ZMusic_MusicStream handle;
	int   baseorder;
	bool  loop;
	FString	 LastSong;			// last music that was played
};

extern MusPlayingInfo mus_playing;

extern float relative_volume, saved_relative_volume;


#endif
