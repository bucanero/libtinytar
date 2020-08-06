#include <untar.h>
#include <string.h>
#include <stdio.h>


int main(void)
{
    // Extract .tar archive contents to /dev_hdd0/tmp/
    untar("/dev_hdd0/tmp/archive.tar", "/dev_hdd0/tmp");

    // Extract .tar.gz archive contents to /dev_hdd0/tmp/
    untar_gz("/dev_hdd0/tmp/archive.tgz", "/dev_hdd0/tmp");

    // Extract .tar.bz2 archive contents to /dev_hdd0/tmp/
    untar_bz2("/dev_hdd0/tmp/archive.tar.bz2", "/dev_hdd0/tmp");

    return 0;
}
