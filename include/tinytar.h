#ifndef UNTAR_EXTRACTOR_H
#define UNTAR_EXTRACTOR_H

/*
	void callback(const char* fileName, unsigned long fileSize, char fileType);
*/
typedef void (*tar_callback_t)(const char*, unsigned long, char);

int untarEx(const char* srcFile, const char* dstPath, tar_callback_t callback);
int untarEx_gz(const char* srcFile, const char* dstPath, tar_callback_t callback);
int untarEx_bz2(const char* srcFile, const char* dstPath, tar_callback_t callback);

int untar(const char* srcFile, const char* dstPath);
int untar_gz(const char* srcFile, const char* dstPath);
int untar_bz2(const char* srcFile, const char* dstPath);

int tarEx(const char* dstFile, const char* srcPath, tar_callback_t callback);
int tarEx_gz(const char* dstFile, const char* srcPath, tar_callback_t callback);
int tarEx_bz2(const char* dstFile, const char* srcPath, tar_callback_t callback);

int tar(const char* dstFile, const char* srcPath);
int tar_gz(const char* dstFile, const char* srcPath);
int tar_bz2(const char* dstFile, const char* srcPath);

#endif //UNTAR_EXTRACTOR_H
