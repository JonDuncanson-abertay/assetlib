#include <include/wav_utils.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct RIFF_HEADER
{
	unsigned char riff_id[4];
	long file_size;
};

struct RIFF_CHUNK_INFO
{
	char chunk_id[4];
	long chunk_size;
};

struct WAV_FMT_CHUNK
{
	unsigned char riff_chunk_id[4];
	long chunk_size;
	int audio_format;
	int channels;
	long sample_rate;
	long byte_rate;
	int block_align;
	int bits_per_sample;
};

struct WAV_DATA_CHUNK
{
	unsigned char riff_chunk_id[4];
	long chunk_size;
	unsigned char* data;
};

bool isRIFFChunkID(unsigned char* const file_buffer, const unsigned int offset, const char* id);

int getRIFFChunkSize(unsigned char* const file_buffer, const unsigned int offset);

bool is_wav(const char* filepath)
{
	FILE* file = fopen(filepath, "rb");

	if (file == NULL)
		return false;

	unsigned char* file_buffer = (unsigned char*)malloc(sizeof(struct RIFF_HEADER) + 4);

	fread(file_buffer, sizeof(struct RIFF_HEADER) + 4, 1, file);

	fclose(file);

	struct RIFF_HEADER header;

	memcpy(&header, file_buffer, sizeof(struct RIFF_HEADER));
	char format[4];
	memcpy(format, file_buffer + sizeof(struct RIFF_HEADER), 4);

	if (strncmp(format, "WAVE", 4) != 0)
	{
		free(file_buffer);
		return false;
	}

	free(file_buffer);
    return true;
}

bool load_wav(const char* filepath, struct audio_data* data)
{
	FILE* file = fopen(filepath, "rb");

	if (file == NULL)
		return false;

	if (!is_wav(filepath))
	{
		fclose(file);
		return false;
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	unsigned char* file_buffer = (unsigned char*)malloc(size);
	if (file_buffer == NULL)
		return false;

	fread(file_buffer, size, 1, file);

	fclose(file);

	struct RIFF_HEADER header;
	struct WAV_FMT_CHUNK fmt;
	struct WAV_DATA_CHUNK raw_data;

	unsigned int pointer_tracker = 0;

	do
	{
		//if the chunk is the RIFF header for the file
		if (isRIFFChunkID(file_buffer, pointer_tracker, "RIFF"))
		{
			memcpy(&header, file_buffer + pointer_tracker, sizeof(struct RIFF_HEADER));
			char format[4];
			memcpy(format, file_buffer + pointer_tracker + sizeof(struct RIFF_HEADER), 4);
			if (strncmp(format, "WAVE", 4) != 0)
			{
				free(file_buffer);
				return -3;
			}
			pointer_tracker += sizeof(struct RIFF_HEADER) + 4;
		}

		//if the chunk is the format information for the WAV file
		else if (isRIFFChunkID(file_buffer, pointer_tracker, "fmt "))
		{
			memcpy(&fmt, file_buffer + pointer_tracker, sizeof(struct WAV_FMT_CHUNK));
			pointer_tracker += sizeof(struct WAV_FMT_CHUNK);
		}

		//if the chunk contains the raw data
		else if (isRIFFChunkID(file_buffer, pointer_tracker, "data"))
		{
			memcpy(&raw_data, file_buffer + pointer_tracker, (sizeof(unsigned char) * 4) + sizeof(long));
			int chunk_size = getRIFFChunkSize(file_buffer, pointer_tracker);
			raw_data.data = (unsigned char*)malloc(chunk_size);
			memcpy(raw_data.data, file_buffer + pointer_tracker + (sizeof(unsigned char) * 4) + sizeof(long), chunk_size);
			pointer_tracker += sizeof(struct RIFF_HEADER) + 4 + chunk_size;
		}

		//else ignore the chunk but move the tracker
		else
		{
			pointer_tracker += sizeof(struct RIFF_HEADER) + getRIFFChunkSize(file_buffer, pointer_tracker);
		}
	} while (pointer_tracker < size - (sizeof(struct RIFF_HEADER) + sizeof(struct WAV_FMT_CHUNK)));

	free(file_buffer);

	data->bits_per_sample = fmt.bits_per_sample;
	data->block_align = fmt.block_align;
	data->byte_rate = fmt.byte_rate;
	data->channels = fmt.channels;
	data->data = raw_data.data;
	data->sample_rate = fmt.sample_rate;

    return true;
}


bool isRIFFChunkID(unsigned char* const file_buffer, const unsigned int offset, const char* id)
{
	char riff_id[4];
	memcpy(riff_id, file_buffer + offset, 4);
	if (strncmp(riff_id, id, 4) != 0)
		return false;
	return true;
}

int getRIFFChunkSize(unsigned char* const file_buffer, const unsigned int offset)
{
	struct RIFF_CHUNK_INFO info;
	memcpy(&info, file_buffer + offset, sizeof(struct RIFF_CHUNK_INFO));
	if (info.chunk_size > 0)
		return info.chunk_size;
	return 0;
}
