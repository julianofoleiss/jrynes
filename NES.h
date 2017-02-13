#ifndef NES_H
#define	NES_H

#define NES_CYCLES_PER_SCANLINE 114
#define NES_NUM_SCANLINES       240

void NESrun(char* Filename);

void TerminateEmulation();

#endif	/* NES_H */
