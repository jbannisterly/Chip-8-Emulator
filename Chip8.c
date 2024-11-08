#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <Windows.h>
#include <string.h>
#include <unistd.h>

bool KeyPressed(uint8_t key);
void Display(uint64_t *disp, char *display, HANDLE hnd, COORD position);
void Instruction(uint16_t code, uint8_t *reg, uint64_t *disp, uint16_t *pc, uint16_t *regI, uint16_t **stack, uint8_t *memory, uint8_t *timer, uint8_t *sound);

int main(int argc, char *args[]){
  uint8_t memory[4096] = {0};
  uint64_t disp[32] = {0};
  uint16_t regI = 0;
  uint8_t reg[16] = {0};
  uint16_t pc = 0x200;
  uint16_t sk[16] = {0};
  uint16_t *stack = sk;
  uint16_t code = 0;
  uint64_t oldClock = clock();
  uint64_t newClock = clock();
  uint8_t timer = 0;
  uint8_t sound = 0;
  char display[65*32+1] = {0};
  uint8_t digits[16 * 5] = {
	  0xF0, 0x90, 0x90, 0x90, 0xF0,
	  0x20, 0x60, 0x20, 0x20, 0x70,
	  0xF0, 0x10, 0xF0, 0x80, 0xF0,
	  0xF0, 0x10, 0xF0, 0x10, 0xF0,
	  0x90, 0x90, 0xF0, 0x10, 0x10,
	  0xF0, 0x90, 0xF0, 0x10, 0xF0,
	  0xF0, 0x80, 0xF0, 0x90, 0xF0,
	  0xF0, 0x10, 0x20, 0x40, 0x40,
	  0xF0, 0x90, 0xF0, 0x90, 0xF0,
	  0xF0, 0x90, 0xF0, 0x10, 0xF0,
	  0xF0, 0x90, 0xF0, 0x90, 0x90,
	  0xE0, 0x90, 0xE0, 0x90, 0xE0,
	  0xF0, 0x80, 0x80, 0x80, 0xF0,
	  0xE0, 0x90, 0x90, 0x90, 0xE0,
	  0xF0, 0x80, 0xF0, 0x80, 0xF0,
	  0xF0, 0x80, 0xF0, 0x80, 0x80,
	  };

  for (int i = 0; i < 80; i++) memory[i + 0x100] = digits[i];
  
  FILE *file = fopen(args[1], "r");
  int size = fread(memory + 0x200, 1, 4096, file);
  
  /*
  printf("%d \n", size);
  printf("MEMORY\n");
  for (int i = 0; i < 4096; i++){
    if (i % 16 == 0) printf("\n");
	printf("%04x ", memory[i]);
  }
  printf("END MEMORY\n");
  */
  
  COORD position;
  HANDLE hnd;
  position.X = 1;
  position.Y = 1;
  hnd = GetStdHandle(-11);

    
  int count = 0;
	
  while(1){
	code = ((uint16_t)memory[pc] << 8) + memory[pc + 1];
	//printf("Code %04x\n", code);
	pc += 2;
    Instruction(code, reg, disp, &pc, &regI, &stack, memory, &timer, &sound);
	count ++;
	newClock = clock();
	if(newClock - oldClock > 16){
	  oldClock = newClock;
	  timer = timer > 0 ? timer - 1: 0;
	  sound = sound > 0 ? sound - 1: 0;
	  Display(disp, display, hnd, position);
	  printf("\n\n\n");
	  printf("Instructions per frame: %06d", count);
	  count = 0;
	}
  }
  return 0;
}

void Display(uint64_t *disp, char *display, HANDLE hnd, COORD position){
  char character;
  SetConsoleCursorPosition(hnd,position);
  for (int i = 0; i < 32; i++){
    for (int j = 63; j >= 0; j--){
	  display[i * 65 + 63 - j] = ((disp[i] >> j) & 0x1) > 0 ? '#' : ' ';
	}
	display[i * 65 + 64] = '\n';
  }
  fwrite(display,sizeof(char),strlen(display),stdout);
}

void Instruction(uint16_t code, uint8_t *reg, uint64_t *disp, uint16_t *pc, uint16_t *regI, uint16_t **stack, uint8_t *memory, uint8_t *timer, uint8_t *sound){
  uint8_t x = (code >> 8) & 0xF;
  uint8_t y = (code >> 4) & 0xF;
  uint16_t NNN = code & 0xFFF;
  uint8_t NN = code & 0xFF;
  uint8_t N = code & 0xF;
  uint8_t flag = 0;
  switch (code >> 12){
    case 0x0:
	  if (y == 0xE && N == 0xE){
	    (*stack)--;
		*pc = **stack;
	  }
	  else if (y == 0xE && N == 0x0){
	    for(int i = 0; i < 32; i++){
		  disp[i] = 0;
		}
	  }
	  break;
	case 0x1:
	  *pc = NNN;
	  break;
	case 0x2:
	  **stack = *pc;
	  (*stack)++;
	  *pc = NNN;
	  break;
	case 0x3:
	  *pc = (reg[x] == NN) ? *pc + 2 : *pc; break;
	case 0x4:
	  *pc = (reg[x] != NN) ? *pc + 2 : *pc; break;
	case 0x5:
	  *pc = (reg[x] == reg[y]) ? *pc + 2: *pc; break;
	case 0x6:
	  reg[x] = NN;
	  break;
	case 0x7:
	  reg[x] += NN;
	  break;
	case 0x8:
	  switch(N){
	    case 0x0: reg[x] = reg[y]; break;
		case 0x1: reg[x] = reg[x] | reg[y]; break;
	    case 0x2: reg[x] = reg[x] & reg[y]; break;
	    case 0x3: reg[x] = reg[x] ^ reg[y]; break;
	    case 0x4: 
		  flag = (((reg[x] + reg[y]) & 0xFF) < reg[x]) ? 1 : 0;
		  reg[x] = reg[x] + reg[y];
		  reg[0xF] = flag; break;
	    case 0x5: 
		  flag = (reg[x] < reg[y]) ? 0 : 1;
		  reg[x] = reg[x] - reg[y];
		  reg[0xF] = flag; break;
	    case 0x6: 
		  flag = (reg[y] & 1);
		  reg[x] = reg[y] >> 1;
		  reg[0xF] = flag; break;
	    case 0x7: 
		  flag = (reg[y] < reg[x]) ? 0 : 1;
		  reg[x] = reg[y] - reg[x];
		  reg[0xF] = flag; break;
	    case 0xE: 
		  flag = (reg[y] >> 7) & 1;
		  reg[x] = reg[y] << 1;
		  reg[0xF] = flag; break;
	  }
	  break;
	case 0x9:
	  *pc = reg[x] != reg[y] ? *pc + 2 : *pc; break;
	case 0xA:
	  *regI = NNN; break;
	case 0xB:
	  *pc = NNN + reg[0]; break;
	case 0xC:
	  reg[x] = rand() & NN; break;
	case 0xD:
	  uint64_t mask;
	  uint8_t xPos;
	  uint8_t yPos;
	  uint8_t collision = 0;
	  xPos = reg[x] & 63;
      yPos = reg[y] & 31;
	  N = 32 - yPos < N ? 32 - yPos : N; 
	  for (int i = 0; i < N; i++){
	    mask = (uint64_t)memory[*regI + i];
		mask = (mask << (56 - xPos)) | (mask >> (xPos - 56));
		collision = collision + (((disp[(yPos + i) % 32] & mask) != 0) ? 1 : 0);
		disp[(yPos + i) % 32] ^= mask;
	  }
	  reg[0xF] = collision > 0 ? 1 : 0;
	  break;
	case 0xE:
	  switch(NN){
	    case 0x9E:
		  *pc = KeyPressed(reg[x]) ? *pc + 2 : *pc; break;
		case 0xA1:
		  *pc = KeyPressed(reg[x]) ? *pc : *pc + 2; break;
	  }
	  break;
	case 0xF:
	  switch(NN){
	    case 0x07: reg[x] = *timer; break;
		case 0x0A: 
		  uint8_t key = 0;
		  int success = 0;
		  while(success == 0){
		    if (KeyPressed(key)){
			  success = 1;
			}else{
			  key = (key + 1) % 16;
			}
		  }
		  reg[x] = key;
		  break;
		case 0x15: *timer = reg[x]; break;
		case 0x18: *sound = reg[x]; break;
		case 0x1E: *regI = *regI + reg[x]; break;
		case 0x29: *regI = 5 * reg[x] + 0x100; break;
		case 0x33:
		  memory[*regI] = reg[x] / 100;
		  memory[*regI + 1] = (reg[x] / 10) % 10;
		  memory[*regI + 2] = reg[x] % 10;
		  break;
		case 0x55:
		  for (int i = 0; i <= x; i++){
		    memory[*regI + i] = reg[i];
		  }
		  *regI += x + 1;
		  break;
		case 0x65:
		  for(int i = 0; i <= x; i++){
		    reg[i] = memory[*regI + i];
		  }
		  *regI += x + 1;
		break;
	  } 
	  break;
  }
  
}

  bool KeyPressed(uint8_t key){
	int vKey;
	/*
	if (key < 10){
	  vKey = key + 0x30;
	}else{
	  vKey = key + 0x41 - 10;
	}
	*/
	return (int16_t)GetAsyncKeyState("X123QWEASDZC4RFV"[key]) < 0; 
  }