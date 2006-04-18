#ifndef __METADATA_H__
#define __METADATA_H__
#include "plugin.h"
typedef enum {
	META_ALBUM_ART = 1, 	/* Album Cover art 	*/
	META_ARTIST_ART = 2, 	/* Artist  image 	*/
	META_ALBUM_TXT = 4,	/* Album story 		*/
	META_ARTIST_TXT = 8, 	/* Artist biography 	*/
	META_SONG_TXT	= 16	/* Lyrics 		*/
}MetaDataType;

typedef enum {
	META_DATA_AVAILABLE,
	META_DATA_UNAVAILABLE,
	META_DATA_FETCHING
} MetaDataResult;

typedef gboolean (*MetaDataCallback)(mpd_Song *song, MetaDataResult result, char *path, gpointer data);
void meta_data_get_path_callback(mpd_Song *song, MetaDataType type, MetaDataCallback callback, gpointer data);

void meta_data_init();

#endif
