/* lsringmenu.c
   This lists all ring menu items of the US SoE ROM.
   The meaning of the action id depends on which code executes the menu.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

/* NOTE: these defines may be different for non-US */
#define LIST_START 0x0e8000 /* start of ring menu item pointer list */
#define LIST_END   0x0e8143 /* end of ring menu item pointer list */
#define ITEM_START 0x0e0000 /* base address for ring menu items pointers */
#define TEXT_START 0x040000 /* base address for text pointers */
#define ICON_START 0x040000 /* base address for icon pointers */

uint8_t f_read8(FILE* f, size_t pos)
{
    fseek(f, pos, SEEK_SET);
    uint8_t res=0;
    fread(&res, 1, 1, f);
    return res;
}
uint16_t f_read16(FILE* f, size_t pos)
{
    fseek(f, pos, SEEK_SET);
    uint8_t buf[2] = {0,0};
    fread(buf, 1, 2, f);
    uint16_t res = buf[1]; res <<= 8;
    res += buf[0];
    return res;
}
uint32_t f_read24(FILE* f, size_t pos)
{
    fseek(f, pos, SEEK_SET);
    uint8_t buf[3] = {0,0,0};
    fread(buf, 1, 3, f);
    uint32_t res = buf[2]; res<<=8;
    res += buf[1]; res<<=8;
    res += buf[0];
    return res;
}
void f_read_text(FILE* f, size_t pos, char* buf, size_t buflen)
{
    fseek(f, pos, SEEK_SET);
    for (size_t i=0; buflen>1 && i<buflen-1; i++) {
        if (fread(buf, 1, 1, f) != 1) {
            *buf = 0;
            return;
        }
        if (*buf == 0) return;
        buf++;
    }
    *buf=0;
}

int main(int argc, char** argv)
{
    if (argc<2) {
        printf("Usage: %s path/to/rom.sfc\n", argv[0]);
        return 1;
    }
    
    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", argv[1]);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    uint32_t off = 512 * ((ftell(f)%512)&1); // allow for 512B header

#define read8(pos) f_read8(f, pos+off)
#define read16(pos) f_read16(f, pos+off)
#define read24(pos) f_read24(f, pos+off)
#define read_text(pos,buf,buflen) f_read_text(f, pos+off, buf, buflen)
    
    printf(" No. %-32s Icon   Action TextP  ItemP   IconAniP\n", "Text");
    for (uint32_t p=LIST_START; p<LIST_END; p+=2) {
        uint32_t q = ITEM_START + read16(p);
        /* at q:
            2 bytes text pointer (offset to TEXT_START)
            2 bytes icon pointer (actually pointer to animation script, see stream-doc 2020-12-03)
            2 bytes flags an palette (icon does not have a pallette attached?)
            2 bytes action id (this is basically spell_nr*2 for spells)
        */
        uint32_t textp = TEXT_START + read16(q+0);
        uint32_t iconp = ICON_START + read16(q+2);
        uint32_t animp = read24(iconp); /* pointer to animation script */
        uint16_t action = read16(q+6);
        char text[33] = {0};
        read_text(textp, text, sizeof(text));
        printf("%3u: %-32s #$%04x #$%02x   #$%04x $%06x $%06x\n", (p-LIST_START)/2, text, iconp-ICON_START, action, textp-TEXT_START, 0x800000|q, animp);
    }
    
    fclose(f);
}
