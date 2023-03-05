#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MD3NAME 64

typedef struct md3tag_s {
	char name[MD3NAME];
	float origin[3];
	float rotationmatrix[9];
} md3tag_t;

typedef struct md3modelheader_s {
	char identifier[4]; // "IDP3"
	int version; // 15
	char name[MD3NAME];
	int flags;
	int num_frames;
	int num_tags;
	int num_meshes;
	int num_skins;
	int lump_frameinfo;
	int lump_tags;
	int lump_meshes;
	int lump_end;
} md3modelheader_t;

void write_little_long(unsigned char *buffer, unsigned int i) {
	buffer[0] = i & 0xFF;
	buffer[1] = (i >> 8) & 0xFF;
	buffer[2] = (i >> 16) & 0xFF;
	buffer[3] = (i >> 24) & 0xFF;
}

void write_little_float(unsigned char *buffer, float f) {
	union {
		float f;
		unsigned int i;
	} tmp;
	tmp.f = f;
	write_little_long(buffer, tmp.i);
}

int read_little_long(int i) {
	const unsigned char *buffer = (void *)&i;
	return (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
}

float read_little_float(float f) {
	union {
		float f;
		int i;
	} tmp;
	tmp.f = f;
	tmp.i = read_little_long(tmp.i);
	return tmp.f;
}

void matrix_normalize(float *matrix) {
	double scale;
	int i;
	for (i = 0;i < 3;i++) {
		scale = sqrt(matrix[i * 3 + 0] * matrix[i * 3 + 0] + matrix[i * 3 + 1] * matrix[i * 3 + 1] + matrix[i * 3 + 2] * matrix[i * 3 + 2]);
		if (scale)
			scale = 1.0 / scale;

		matrix[i * 3 + 0] *= scale;
		matrix[i * 3 + 1] *= scale;
		matrix[i * 3 + 2] *= scale;
	}
}

#define BUFFER_SIZE (64*1024*1024)

int main(int argc, char **argv) {
	int tag_count, i, j;
	const char *inpath, *outpath;
	unsigned char *buffer = malloc(64 * 1024 * 1024);
	FILE *infile = NULL;
	FILE *outfile = NULL;
	char name[MD3NAME + 1];
	float rotationmatrix[9];
	double det;
	int filesize;
	int r = 1;
	int frames;
	int n;
	md3tag_t *tags;
	if (!buffer) {
		perror("malloc");
		goto finish;
	}
	md3modelheader_t *header;
	if (argc != 3) {
		printf("Usage: %s [inputfile] [outputfile]\n", argv[0]);
		goto finish;
	}
	inpath = argv[1];
	outpath = argv[2];
	infile = fopen(inpath, "rb");
	if ((filesize = fread(buffer, 1, BUFFER_SIZE, infile)) <= 0) {
		if (filesize < 0)
			perror("fread");
		else
			fprintf(stderr, "input file is empty\n");

		goto finish;
	}
	if (!feof(infile)) {
		fprintf(stderr, "complete read of file failed, %i bytes was readed\n", filesize);
		goto finish;
	}
	fclose(infile);
	infile = NULL;
	header = (void *)buffer;
	tag_count = read_little_long(header->num_tags);
	frames = read_little_long(header->num_frames);
	printf("Amount of tags: %i\n", tag_count);
	printf("Amount of frames: %i\n", frames);
	tags = (md3tag_t *)(buffer + read_little_long(header->lump_tags));
	n = tag_count * frames;
	for (i = 0; i < n; i++) {
		memcpy(name, tags[i].name, MD3NAME);
		name[MD3NAME] = 0;
		printf("tag name: %s\n", name);
		for (j = 0; j < 9; j++)
			rotationmatrix[j] = read_little_float(tags[i].rotationmatrix[j]);

		printf("rotation matrix:\n   %f %f %f\n   %f %f %f\n   %f %f %f\n", rotationmatrix[0], rotationmatrix[1], rotationmatrix[2], rotationmatrix[3], rotationmatrix[4], rotationmatrix[5], rotationmatrix[6], rotationmatrix[7], rotationmatrix[8]);
		matrix_normalize(rotationmatrix);
		printf("normalized rotation matrix:\n   %f %f %f\n   %f %f %f\n   %f %f %f\n", rotationmatrix[0], rotationmatrix[1], rotationmatrix[2], rotationmatrix[3], rotationmatrix[4], rotationmatrix[5], rotationmatrix[6], rotationmatrix[7], rotationmatrix[8]);
		for (j = 0; j < 9; j++)
			write_little_float((unsigned char *)&(tags[i].rotationmatrix[j]), rotationmatrix[j]);
	}
	outfile = fopen(outpath, "wb");
	if (fwrite(buffer, 1, filesize, outfile) < filesize) {
		perror("fwrite");
		goto finish;
	}
	r = 0;
finish:
	if (infile)
		fclose (infile);

	if (outfile)
		fclose (outfile);

	if (buffer)
		free(buffer);

	return r;
}
