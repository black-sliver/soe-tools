#include <string>
#include <stdio.h>
#include <stddef.h>

std::string hex(uint32_t val)
{
    char buf[64]; snprintf(buf, sizeof(buf), "%02x", (unsigned)val);
    return std::string(buf);
}
std::string buf_get_text(const uint8_t* buf, uint32_t addr, uint32_t len)
{
    std::string data = "";
    std::string info = "";
    addr = read24(addr);
    bool mode = (addr & 0x800000);
    addr = 0xc00000 + (addr & 0x7fff) + ((addr & 0x7f8000)<<1);
    if (addr>0xcfffff) return "";
    #if 1
    info += hex(addr);
    info += "> ";
    #endif
    
    if (mode) {
        uint8_t next_plain = 0;
        do {
            uint8_t d = read8(addr);
            if (next_plain) {
                next_plain--;
                if (!d && !next_plain) break;
                data += (char)d;
            } else if (d==0xc0) {
                addr++;
#ifdef TEXT_SHOW_DICT_USE
                data += '(';
                data += hex(read8(addr));
                data += ')';
#endif
                uint32_t wordpp = 0x91f46c + ((uint16_t)read8(addr)<<1);
                uint32_t wordp = 0x91f7d5 + read16(wordpp);
                for (char c; c=read8(wordp); wordp++)  {
                    data += c;
                }
            } else if ((d&0xc0)==0xc0) {
#ifdef TEXT_SHOW_DICT_USE
                data += '(';
                data += hex(d);
                data += ')';
#endif
                d = (d<<1)&0x7e;
                uint32_t wordpp = 0x91f3ec + d;
                uint32_t wordp = 0x91f66c + read16(wordpp);
                for (char c; c=read8(wordp); wordp++)  {
                    data += c;
                }
            } else if ((d&0xc0)==0x40) {
                next_plain = d&0x3f;
            } else if ((d&0xc0)==0x80) {
                d<<=1;
                char c;
                c = (char)read8(0x91f32e + d);
                data += c;
                c = (char)read8(0x91f32f + d);
                if (!c) break;
                data += c;
            } else if ((d&0xc0)==0) {
                char c = (char)read8(0x91f3ae + d);
                if (!c) break;
                data += c;
            }
            addr++;
        } while (true);
    } else {
        for (char c; c=read8(addr); addr++)  {
            data += c;
        }
    }
    
    // make raw data look nice
    std::string printable = "\"";
    char c1=0, c2=0;
    size_t n=0, m=0;
    for (auto c: data) {
        if (m>0 && ((uint8_t)c == 0x96 || (uint8_t)c == 0x97)) {
            printable += "\"\n\"";
            n=0; m=0;
        }
        if ((uint8_t)c==0x80 && (uint8_t)c2==0x80) {
            // c1 = duration?
            if (c1>=0x20 && c1<0x7f)
                printable.erase(printable.length()-7, 7);
            else
                printable.erase(printable.length()-12, 12);
            printable+="<PAUSE>";
            m += 6;
        }
        else if (c>=0x20 && c<0x7f) printable+=c;
        else if (c=='\n') { printable+="<LF>"; m+=3; }
        else { printable += "<0x" + hex((uint8_t)c) + ">"; m+=5; }
        n++; m++;
        if (((c==' ' || (unsigned)c == 0x80) && (n>= 80 || m>=80)) || c=='\n') {
            printable += "\"\n\"";
            n=0; m=0;
        }
        c2 = c1;
        c1 = c;
    }
    printable += '"';
    
    return info+printable;
}

#if 0 // sample use
    case 0x51: // set text from word list in next two bytes + 91d000
        addr = 0x91d000 + read16(scriptaddr); scriptaddr+=2;
        printf("%s[" ADDRFMT "] (%02x) SHOW TEXT %04x FROM 0x%06x %s WINDOWED, WAIT FOR B-PRESS\n"
               "%s                %s\n",
            spaces, ADDR, instr, read16(scriptaddr-2), addr,
            read24(addr)&0x800000 ? "compressed" : "uncompressed",
            spaces, get_text(addr).c_str());
#endif
