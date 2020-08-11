/*
 * tarball.c
 *
 *  Created on: Jul 28, 2010
 *      Author: Pierre Lindenbaum PhD
 *              plindenbaum@yahoo.fr
 *              http://plindenbaum.blogspot.com
 *              
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <zlib.h>
#include <tar.h>

#include "bzlib.h"
#include "tinytar.h"
#include "tardata.h"

#define Print(...)

typedef int (*write_func_t)(void*, char*, int);

void* archive = NULL;
write_func_t writer = NULL;
tar_callback_t user_callback = NULL;

void Tar_init(void* arc, write_func_t wf, tar_callback_t cb)
{
	archive = arc;
	writer = wf;
	user_callback = cb;
}

void Tar_initRecord(tar_block_t* header, char type)
{
    memset(header, 0, sizeof(tar_block_t));
    sprintf(header->magic, TMAGIC);
    sprintf(header->mtime, "%011lo", time(NULL));
    sprintf(header->mode, "%07o", (type == REGTYPE ? 0644 : 0755));
    sprintf(header->uname, "tinytar");
    sprintf(header->gname, "users");
    memcpy(header->version, TVERSION, TVERSLEN);
    header->typeflag = type;
}

void Tar_checksum(tar_block_t* header)
{
    unsigned int sum = 0;
    char *p = (char *) header;
    char *q = p + sizeof(tar_block_t);
    while (p < header->checksum) sum += *p++ & 0xff;
    for (int i = 0; i < 8; ++i)  {
	  sum += ' ';
	  ++p;
	}
    while (p < q) sum += *p++ & 0xff;

    sprintf(header->checksum, "%06o", sum);
}

void Tar_size(tar_block_t* header, size_t fileSize)
{
    sprintf(header->size, "%011llo", (long long unsigned int)fileSize);
}

void Tar_filename(tar_block_t* header, const char* filename)
{
    if(filename==NULL || filename[0]==0 || strlen(filename)>=100)
	{
		Print("invalid archive name '%s'", filename);
		return;
	}
    snprintf(header->name, 100, "%s", filename);
}

void Tar_endRecord(size_t len)
{
    while((len % sizeof(tar_block_t)) != 0)
    {
	    writer(archive, "\0", 1);
	    ++len;
    }
}

/** writes 2 empty blocks. Should be always called before closing the Tar file */
void Tar_finish()
{
    //The end of the archive is indicated by two blocks filled with binary zeros
    tar_block_t header;
    memset(&header, 0, sizeof(tar_block_t));
    writer(archive, (char*) &header, sizeof(tar_block_t));
    writer(archive, (char*) &header, sizeof(tar_block_t));
}

void Tar_putBuffer(const char* filename, char type, const char* content, size_t len)
{
    tar_block_t header;
    Tar_initRecord(&header, type);
    Tar_filename(&header, filename);
    Tar_size(&header, len);
    Tar_checksum(&header);
    writer(archive, (char*) &header, sizeof(tar_block_t));
    if (len) writer(archive, (char*) content, len);
    Tar_endRecord(len);
}

void Tar_putString(const char* filename, const char* content)
{
    Tar_putBuffer(filename, REGTYPE, content, strlen(content));
}

int Tar_putDirectory(const char* dirInArchive)
{
	if (user_callback)
		user_callback(dirInArchive, 0, DIRTYPE);

    Tar_putBuffer(dirInArchive, DIRTYPE, NULL, 0);
	return 0;
}

int Tar_putFile(const char* filename, const char* nameInArchive)
{
    char buff[BUFSIZ];
    FILE* in=fopen(filename, "rb");

    if(!in)
	{
		Print("Cannot open source file %s\n", filename);
		return (-1);
	}
    fseek(in, 0L, SEEK_END);
    long int len= ftell(in);
    fseek(in, 0L, SEEK_SET);

	if (user_callback)
		user_callback(nameInArchive, len, REGTYPE);

    tar_block_t header;
    Tar_initRecord(&header, REGTYPE);
    Tar_filename(&header, nameInArchive);
    Tar_size(&header, len);
    Tar_checksum(&header);
    writer(archive, (char*) &header, sizeof(tar_block_t));

    size_t nRead=0;
    while((nRead = fread(buff, sizeof(char), BUFSIZ, in)) > 0)
	{
		writer(archive, buff, nRead);
	}
    fclose(in);

    Tar_endRecord(len);
    return 0;
}

void walk_tar_directory(const char* startdir, const char* inputdir)
{
	char fullname[256];	
	struct dirent *dirp;
	int len = strlen(startdir) + 1;
	DIR *dp = opendir(inputdir);

	if (!dp) {
		Print("Failed to open input directory: '%s'", inputdir);
		return;
	}

	if (strlen(inputdir) > len)
	{
		Print("Adding folder '%s'", inputdir+len);
		if (Tar_putDirectory(inputdir+len) < 0)
		{
			Print("Failed to add directory to tar: %s", inputdir);
			return;
		}
	}

	while ((dirp = readdir(dp)) != NULL) {
		if ((strcmp(dirp->d_name, ".")  != 0) && (strcmp(dirp->d_name, "..") != 0)) {
  			snprintf(fullname, sizeof(fullname), "%s%s", inputdir, dirp->d_name);

  			if (dirp->d_type == DT_DIR) {
    			strcat(fullname, "/");
    			walk_tar_directory(startdir, fullname);
  			} else {
    			Print("Adding file '%s'", fullname+len);
    			if (Tar_putFile(fullname, fullname+len) < 0) {
      				Print("Failed to add file to tar: %s", fullname);
    			}
  			}
		}
	}
	closedir(dp);
}

int gz_write(void* gzF, char* buf, int len)
{
	return gzwrite(gzF, buf, len);
}

int bz2_write(void* bzF, char* buf, int len)
{
	return BZ2_bzwrite(bzF, buf, len);
}

int std_write(void* stdF, char* buf, int len)
{
	return fwrite(buf, 1, len, stdF);
}

void tar_process(const char* path)
{
	char* indir;
	char* start = strdup(path);
	
	if (path[strlen(path) - 1] == '/')
	{
		indir = strdup(path);
		*strrchr(start, '/') = 0;
	}
	else
	{
		asprintf(&indir, "%s/", path);
	}

	/* add items */
	walk_tar_directory(start, indir);

	/* finalize the tar file */
	Tar_finish();
	free(start);
	free(indir);
}

int tarEx(const char* dstFile, const char* srcPath, tar_callback_t callback)
{
	/* open file for writing */
	FILE *a = fopen(dstFile, "wb");

	if (!a)
		return (-1);

	/* create the tar file */
	Tar_init(a, &std_write, callback);
	tar_process(srcPath);

	/* close the file */
	fclose(a);

	/* we're done */
	return (0);
}

int tarEx_gz(const char* dstFile, const char* srcPath, tar_callback_t callback)
{
	gzFile gz = gzopen(dstFile, "wb");

	if (!gz)
		return (-1);

	Tar_init(gz, &gz_write, callback);
	tar_process(srcPath);

	gzclose_w(gz);
	return (0);
}

int tarEx_bz2(const char* dstFile, const char* srcPath, tar_callback_t callback)
{
	BZFILE *bz = BZ2_bzopen(dstFile, "wb");

	if (!bz)
		return (-1);

	Tar_init(bz, &bz2_write, callback);
	tar_process(srcPath);

	BZ2_bzclose(bz);
	return (0);
}

int tar(const char* dstFile, const char* srcPath)
{
	return tarEx(dstFile, srcPath, NULL);
}

int tar_gz(const char* dstFile, const char* srcPath)
{
	return tarEx_gz(dstFile, srcPath, NULL);
}

int tar_bz2(const char* dstFile, const char* srcPath)
{
	return tarEx_bz2(dstFile, srcPath, NULL);
}
