#include <decode.h>



/* 
 * Given the HEX representation of machine code and the
 * PC value, it decodes the instruction to its assembly equivalent.
 * PC value is needed to calculate the absolute memory address for
 * instructions containing an offset value to be added to 
 * present address considering as base, e.g. CALL.
 * 
 * Bit masking and bit shifting operations have been done
 * according to Chapter - 5 (Instructions) of SPARC v8 manual.
 * Field specifications strictly adheres to Format - I, II, III
 * instruction as described in the section referred.
 * 
 * Decoding of instructions refers to Appendix 'B' - 'Instruction Definitions'
 * section of SPARC v8 manual. Subsections have been referred in form of
 * B.<x>, <x> being a number referring to specific subsection of the appendix.
 */
char* decodeInstruction(char* cpuInstruction, unsigned long regPC)
{
	char* disassembledInstruction = (char*)malloc(50);
	unsigned long instructionWord, hexDigit, op, disp30, rd, a, cond, op2, imm22, disp22, op3, rs1, /*asi, */i, rs2, simm13, opf;
	long sign_extended_simm13, sign_extended_disp22;
	short fsr = 0, fq = 0, csr = 0, cq = 0;
	char* hexNumber = (char*)malloc(32);
	char* opcode = NULL;
	char* address = NULL;
	char* reg_or_imm = NULL;
	
	/* Reads four bytes one by one starting from lowest to highest. Once a byte is read, it is left shifted
         * by 24 bits followed by right shifted by 24 bits to clear higher order 24 bits, if set by sign extension 
         * caused by widening of data during auto-casting. Casting takes place because of hexDigit being an
         * unsigned long (32 bits) while cpuInstruction is an array of type char (8 bits). All four bytes are
         * packed together to form a 32 bit 'instructionWord'.
         */
	instructionWord = 0;
	hexDigit = cpuInstruction[0]; hexDigit = (hexDigit << 24) >> 24; instructionWord = (instructionWord << 8) | hexDigit;
	hexDigit = cpuInstruction[1]; hexDigit = (hexDigit << 24) >> 24; instructionWord = (instructionWord << 8) | hexDigit;
	hexDigit = cpuInstruction[2]; hexDigit = (hexDigit << 24) >> 24; instructionWord = (instructionWord << 8) | hexDigit;
	hexDigit = cpuInstruction[3]; hexDigit = (hexDigit << 24) >> 24; instructionWord = (instructionWord << 8) | hexDigit;
	
	op = instructionWord >> 30; 
        sparc_instruction_fields.op = op;
        sparc_instruction_fields.op2 = 1;       // op2 = 1 is unimplemented
        sparc_instruction_fields.op3 = 0x22;    // op3 = 0x22 is unimplemented
	
	// Format - I instruction
	// CALL
	if(op == 1)
	{
		disp30 = instructionWord & 0x3FFFFFFF;
		sprintf(hexNumber, "%lX", ((disp30 << 2) + regPC));
		strcpy(disassembledInstruction, "call 0x");
		strcat(disassembledInstruction, hexNumber);
	}
	
	else
	// Format - II instruction
    	if(op == 0)
    	{
    		op2 = (instructionWord & 0x01C00000) >> 22; 
                sparc_instruction_fields.op2 = op2;
    		
			if(op2 == 4)
			{				
				rd = (instructionWord & 0x3E000000) >> 25; 
				imm22 = instructionWord & 0x003FFFFF;
				
				// NOP
				if((rd == 0) && (imm22 == 0))
					strcpy(disassembledInstruction, "nop");
				else
				{
					// SETHI
					strcpy(disassembledInstruction, "sethi %hi(0x");
					sprintf(hexNumber, "%lX", (imm22 << 10));
					strcat(disassembledInstruction, hexNumber);
					strcat(disassembledInstruction, "), ");
					strcat(disassembledInstruction, getIntegerRegisterName(rd));
				}
			}
			
			else
			{
				a = (instructionWord & 0x20000000) >> 29;
				cond = (instructionWord & 0x1E000000) >> 25;
				disp22 = instructionWord & 0x003FFFFF;
				sign_extended_disp22 = (disp22 & 0x3FFFFF) | ((disp22 & 0x20000) ? 0xFFC00000 : 0); // Sign extend disp22
				disp22 = sign_extended_disp22;
				
				switch(op2)
				{
					// B.31. Unimplemented Instruction
					case 0: 
					strcpy(disassembledInstruction, "unimp 0x");
					sprintf(hexNumber, "%lX", disp22);
					strcat(disassembledInstruction, hexNumber);
					break;
					
					// B.21. Branch on Integer Condition Codes Instructions
					case 2:
					{
						switch(cond)
						{
							case 0b1000: opcode = "ba";	break;
							case 0b0000: opcode = "bn"; break;
							case 0b1001: opcode = "bne"; break;
							case 0b0001: opcode = "be"; break;
							case 0b1010: opcode = "bg"; break;
							case 0b0010: opcode = "ble"; break;
							case 0b1011: opcode = "bge"; break;
							case 0b0011: opcode = "bl"; break;
							case 0b1100: opcode = "bgu"; break;
							case 0b0100: opcode = "bleu"; break;
							case 0b1101: opcode = "bcc"; break;
							case 0b0101: opcode = "bcs"; break;
							case 0b1110: opcode = "bpos"; break;
							case 0b0110: opcode = "bneg"; break;
							case 0b1111: opcode = "bvc"; break;
							case 0b0111: opcode = "bvs"; break;							
						}
						break;
					}
					
					// B.22. Branch on Floating- point Condition Codes Instructions
					case 6:
					{
						switch(cond)
						{
							case 0b1000: opcode = "fba"; break;
							case 0b0000: opcode = "fbn"; break;
							case 0b0111: opcode = "fbu"; break;
							case 0b0110: opcode = "fbg"; break;
							case 0b0101: opcode = "fbug"; break;
							case 0b0100: opcode = "fbl"; break;
							case 0b0011: opcode = "fbul"; break;
							case 0b0010: opcode = "fblg"; break;
							case 0b0001: opcode = "fbne"; break;
							case 0b1001: opcode = "fbe"; break;
							case 0b1010: opcode = "fbue"; break;
							case 0b1011: opcode = "fbge"; break;
							case 0b1100: opcode = "fbuge"; break;
							case 0b1101: opcode = "fble"; break;
							case 0b1110: opcode = "fbule"; break;							
							case 0b1111: opcode = "fbo"; break;	
						}
						break;
					}
					
					// B.23. Branch on Coprocessor Condition Codes Instructions
					case 7:
					{
						switch(cond)
						{
							case 0b1000: opcode = "cba"; break;
							case 0b0000: opcode = "cbn"; break;
							case 0b0111: opcode = "cb3"; break;
							case 0b0110: opcode = "cb2"; break;
							case 0b0101: opcode = "cb23"; break;
							case 0b0100: opcode = "cb1"; break;
							case 0b0011: opcode = "cb13"; break;
							case 0b0010: opcode = "cb12"; break;
							case 0b0001: opcode = "cb123"; break;
							case 0b1001: opcode = "cb0"; break;
							case 0b1010: opcode = "cb03"; break;
							case 0b1011: opcode = "cb02"; break;
							case 0b1100: opcode = "cb023"; break;
							case 0b1101: opcode = "cb01"; break;
							case 0b1110: opcode = "cb013"; break;
							case 0b1111: opcode = "cb012"; break;							
						}
						break;
					}
				}
				
				if(opcode != NULL)
				{
					strcpy(disassembledInstruction, opcode);
					(a == 1) ? strcat(disassembledInstruction, ",a ") : strcat(disassembledInstruction, " ");
					sprintf(hexNumber, "0x%lX", ((disp22 << 2) + regPC));
					strcat(disassembledInstruction, hexNumber);
				}
			}
    	}
	
	// Format - III instruction
    	else
        {
            rd = (instructionWord & 0x3E000000) >> 25;
            op3 = (instructionWord & 0x01F80000) >> 19; sparc_instruction_fields.op3 = op3;
            rs1 = (instructionWord & 0x00007C000) >> 14;
            i = (instructionWord & 0x00002000) >> 13;
            simm13 = (instructionWord & 0x00001FFF);
            rs2 = instructionWord & 0x0000201F;
            // asi = (instructionWord & 0x00001FE0) >> 5;
            opf = (instructionWord & 0x00003FE0) >> 5; sparc_instruction_fields.opf = opf;
            
            // Sign extend simm13
            sign_extended_simm13 = (simm13 & 0x1FFF) | ((simm13 & 0x1000) ? 0xFFFFE000 : 0);
            simm13 = sign_extended_simm13;
			
			// op = 3
                        if(op == 3)
			{
				switch(op3)
				{
					// B.1. Load Integer Instructions
					case 0b001001: opcode = "ldsb"; break;
					case 0b001010: opcode = "ldsh"; break;
					case 0b000001: opcode = "ldub"; break;
					case 0b000010: opcode = "lduh"; break;
					case 0b000000: opcode = "ld"; break;
					case 0b000011: opcode = "ldd"; break;
					case 0b011001: opcode = "ldsba"; break;
					case 0b011010: opcode = "ldsha"; break;
					case 0b010001: opcode = "lduba"; break;
					case 0b010010: opcode = "lduha"; break;
					case 0b010000: opcode = "lda"; break;
					case 0b010011: opcode = "ldda"; break;
					
					// B.7. Atomic Load-Store Unsigned Byte Instructions
					case 0b001101: opcode = "ldstub"; break;
					case 0b011101: opcode = "ldstub"; break;
					
					// B.8. SWAP Register with Memory Instruction
					case 0b001111: opcode = "swap"; break;
					case 0b011111: opcode = "swapa"; break;
					
				}
				
				if(opcode != NULL)
				{
					strcpy(disassembledInstruction, opcode);
					strcat(disassembledInstruction, " [ ");
					address = getAddress(rs1, rs2, i, simm13, 1);
					strcat(disassembledInstruction, address);
					strcat(disassembledInstruction, " ], ");
					strcat(disassembledInstruction, getIntegerRegisterName(rd));
					opcode = NULL;
				}
				
				
				switch(op3)
				{
					// B.2. Load Floating-point Instructions
					case 0b100000: opcode = "ld"; break;
					case 0b100011: opcode = "ldd"; break;
					case 0b100001: opcode = "ld"; fsr = 1; break;
				}
				
				if(opcode != NULL)
				{
					strcpy(disassembledInstruction, opcode);
					strcat(disassembledInstruction, " [ ");
					address = getAddress(rs1, rs2, i, simm13, 1);
					strcat(disassembledInstruction, address);
					strcat(disassembledInstruction, " ], ");
					if(fsr == 1)
						strcat(disassembledInstruction, "%fsr");
					else
						strcat(disassembledInstruction, getFloatingRegisterName(rd));
					opcode = NULL;
				}
				
				
				switch(op3)
				{
					// B.3. Load Coprocessor Instructions
					case 0b110000: opcode = "ld"; break;
					case 0b110011: opcode = "ldd"; break;
					case 0b110001: opcode = "ld"; csr = 1; break;
				}
				
				if(opcode != NULL)
				{
					strcpy(disassembledInstruction, opcode);
					strcat(disassembledInstruction, " [ ");
					address = getAddress(rs1, rs2, i, simm13, 3);
					strcat(disassembledInstruction, address);
					strcat(disassembledInstruction, " ], ");
					if(csr == 1)
						strcat(disassembledInstruction, "%csr");
					else
						strcat(disassembledInstruction, getCoProcessorRegisterName(rd));
					opcode = NULL;
				}
				
				
				switch(op3)
				{
					// B.4. Store Integer Instructions
					case 0b000101: opcode = "stb"; break;
					case 0b000110: opcode = "sth"; break;
					case 0b000100: opcode = "st"; break;
					case 0b000111: opcode = "std"; break;
					case 0b010101: opcode = "stba"; break;
					case 0b010110: opcode = "stha"; break;
					case 0b010100: opcode = "sta"; break;
					case 0b010111: opcode = "stda"; break;
				}
				
				if(opcode != NULL)
				{
					strcpy(disassembledInstruction, opcode);
					strcat(disassembledInstruction, " ");
					strcat(disassembledInstruction, getIntegerRegisterName(rd));
					strcat(disassembledInstruction, ", [ ");
					address = getAddress(rs1, rs2, i, simm13, 1);
					strcat(disassembledInstruction, address);
					strcat(disassembledInstruction, " ]");					
					opcode = NULL;
				}
				
				
				switch(op3)
				{
					// B.5. Store Floating-point Instructions
					case 0b100100: opcode = "st"; break;
					case 0b100111: opcode = "std"; break;
					case 0b100101: opcode = "st"; fsr = 1; break;
					case 0b100110: opcode = "std"; fq = 1; break;
				}
				
				if(opcode != NULL)
				{
					strcpy(disassembledInstruction, opcode);
					strcat(disassembledInstruction, " ");
					if(fsr == 1)
						strcat(disassembledInstruction, "%fsr");
					else
						if(fq == 1)
							strcat(disassembledInstruction, "%fq");
						else
							strcat(disassembledInstruction, getFloatingRegisterName(rd));
					strcat(disassembledInstruction, ", [ ");
					address = getAddress(rs1, rs2, i, simm13, 1);
					strcat(disassembledInstruction, address);
					strcat(disassembledInstruction, " ]");					
					opcode = NULL;
				}
				
				
				switch(op3)
				{
					// B.6. Store Coprocessor Instructions
					case 0b110100: opcode = "st"; break;
					case 0b110111: opcode = "std"; break;
					case 0b110101: opcode = "st"; csr = 1; break;
					case 0b110110: opcode = "std"; cq = 1; break;
				}
				
				if(opcode != NULL)
				{
					strcpy(disassembledInstruction, opcode);
					strcat(disassembledInstruction, " ");
					if(csr == 1)
						strcat(disassembledInstruction, "%csr");
					else
						if(cq == 1)
							strcat(disassembledInstruction, "%cq");
						else
							strcat(disassembledInstruction, getCoProcessorRegisterName(rd));
					strcat(disassembledInstruction, ", [ ");
					address = getAddress(rs1, rs2, i, simm13, 1);
					strcat(disassembledInstruction, address);
					strcat(disassembledInstruction, " ]");					
					opcode = NULL;
				}
			}
			
			// op = 2
			else
			{
				switch(op3)
				{
					// B.11. Logical Instructions
					case 0b000001: opcode = "and"; break;
					case 0b010001: opcode = "andcc"; break;
					case 0b000101: opcode = "andn"; break;
					case 0b010101: opcode = "andncc"; break;
					case 0b000010: opcode = "or"; break;
					case 0b010010: opcode = "orcc"; break;
					case 0b000110: opcode = "orn"; break;
					case 0b010110: opcode = "orncc"; break;
					case 0b000011: opcode = "xor"; break;
					case 0b010011: opcode = "xorcc"; break;
					case 0b000111: opcode = "xnor"; break;
					case 0b010111: opcode = "xnorcc"; break;
					
					// B.12. Shift Instructions
					case 0b100101: opcode = "sll"; break;
					case 0b100110: opcode = "srl"; break;
					case 0b100111: opcode = "sra"; break;
					
					// B.13. Add Instructions
					case 0b000000: opcode = "add"; break;
					case 0b010000: opcode = "addcc"; break;
					case 0b001000: opcode = "addx"; break;
					case 0b011000: opcode = "addxcc"; break;
					
					// B.14. Tagged Add Instructions
					case 0b100000: opcode = "taddcc"; break;
					case 0b100010: opcode = "taddcctv"; break;
					
					// B.15. Subtract Instructions
					case 0b000100: opcode = "sub"; break;
					case 0b010100: opcode = "subcc"; break;
					case 0b001100: opcode = "subx"; break;
					case 0b011100: opcode = "subxcc"; break;
					
					// B.16. Tagged Subtract Instructions
					case 0b100001: opcode = "tsubcc"; break;
					case 0b100011: opcode = "tsubcctv"; break;
					
					// B.17. Multiply Step Instruction
					case 0b100100: opcode = "mulscc"; break;
					
					// B.18. Multiply Instructions
					case 0b001010: opcode = "umul"; break;
					case 0b001011: opcode = "smul"; break;
					case 0b011010: opcode = "umulcc"; break;
					case 0b011011: opcode = "smulcc"; break;
					
					// B.19. Divide Instructions
					case 0b001110: opcode = "udiv"; break;
					case 0b001111: opcode = "sdiv"; break;
					case 0b011110: opcode = "udivcc"; break;
					case 0b011111: opcode = "sdivcc"; break;
					
					// B.20. SAVE and RESTORE Instructions
					case 0b111100: opcode = "save"; break;
					case 0b111101: opcode = "restore"; break;
				}
				
				if(opcode != NULL)
				{
					strcpy(disassembledInstruction, opcode);
					strcat(disassembledInstruction, " ");
					strcat(disassembledInstruction, getIntegerRegisterName(rs1));		
					strcat(disassembledInstruction, ", ");
					reg_or_imm = getReg_Or_Imm(rs2, i, simm13, 1);
					strcat(disassembledInstruction, reg_or_imm);
					strcat(disassembledInstruction, ", ");
					strcat(disassembledInstruction, getIntegerRegisterName(rd));
					opcode = NULL;
				}
				
				
				switch(op3)
				{
					// B.25. Jump and Link Instruction
					case 0b111000: opcode = "jmpl"; break;
				}
				
				if(opcode != NULL)
				{
					strcpy(disassembledInstruction, opcode);
					strcat(disassembledInstruction, " ");
					address = getAddress(rs1, rs2, i, simm13, 1);
					strcat(disassembledInstruction, address);
					strcat(disassembledInstruction, ", ");
					strcat(disassembledInstruction, getIntegerRegisterName(rd));
					opcode = NULL;
				}
				
				
                                switch(op3)
				{
					// B.28. Read State Register Instructions
					case 0b101000: opcode = "rd"; break;
					case 0b101001: opcode = "rd"; break;
					case 0b101010: opcode = "rd"; break;
					case 0b101011: opcode = "rd"; break;
				}

				if(opcode != NULL)
				{
					strcpy(disassembledInstruction, opcode);
					strcat(disassembledInstruction, " ");
                                        
                                        switch(op3)
					{
						case 0b101000: 
                                                {
                                                    if(rs1 == 0)    // op3 == 0x28 is implicit in case being 0b101000
                                                        strcat(disassembledInstruction, "%y"); 
                                                    else
                                                    {
                                                        char* asrRegister = (char*)malloc(3);
                                                        strcat(disassembledInstruction, "%asr");
                                                        sprintf(asrRegister, "%ld", rs1);
                                                        strcat(disassembledInstruction, asrRegister); 
                                                    }
                                                    break;
                                                }
						case 0b101001: strcat(disassembledInstruction, "%psr"); break;
						case 0b101010: strcat(disassembledInstruction, "%wim"); break;
						case 0b101011: strcat(disassembledInstruction, "%tbr"); break;
					}
                                        
					strcat(disassembledInstruction, ", ");
                                        strcat(disassembledInstruction, getIntegerRegisterName(rd));
					opcode = NULL;
				}

                                
				switch(op3)
				{
					// B.29. Write State Register Instructions
					case 0b110000: opcode = "wr"; break;
					case 0b110001: opcode = "wr"; break;
					case 0b110010: opcode = "wr"; break;
					case 0b110011: opcode = "wr"; break;
				}

				if(opcode != NULL)
				{
					strcpy(disassembledInstruction, opcode);
					strcat(disassembledInstruction, " ");
					strcat(disassembledInstruction, getIntegerRegisterName(rs1));
					strcat(disassembledInstruction, ", ");
					reg_or_imm = getReg_Or_Imm(rs2, i, simm13, 1);
					strcat(disassembledInstruction, reg_or_imm);
					strcat(disassembledInstruction, ", ");

					switch(op3)
					{
						case 0b110000: 
                                                {
                                                    if(rd == 0 && op3 == 0x30)    
                                                        strcat(disassembledInstruction, "%y"); 
                                                    else
                                                    {
                                                        char* asrRegister = (char*)malloc(3);
                                                        strcat(disassembledInstruction, "%asr");
                                                        sprintf(asrRegister, "%ld", rd);
                                                        strcat(disassembledInstruction, asrRegister); 
                                                    }
                                                    break;
                                                }
						case 0b110001: strcat(disassembledInstruction, "%psr"); break;
						case 0b110010: strcat(disassembledInstruction, "%wim"); break;
						case 0b110011: strcat(disassembledInstruction, "%tbr"); break;
					}
					opcode = NULL;
				}

                                
                                switch(op3)
				{
					// B.33. Floating point operate (FPop) Instructions
					case 0b110100:  // Fpop1
                                        {
                                            switch(opf)
                                            {
                                                case 0b001000001: opcode = "fadds"; break;
                                                case 0b001000010: opcode = "faddd"; break;
                                                case 0b001000101: opcode = "fsubs"; break;
                                                case 0b001000110: opcode = "fsubd"; break;
                                                case 0b001001001: opcode = "fmuls"; break;
                                                case 0b001001010: opcode = "fmuld"; break;
                                                case 0b001001101: opcode = "fdivs"; break;
                                                case 0b001001110: opcode = "fdivd"; break;
                                            }
                                            
                                            if(opcode != NULL)
                                            {
                                                    strcpy(disassembledInstruction, opcode);
                                                    strcat(disassembledInstruction, " ");
                                                    strcat(disassembledInstruction, getFloatingRegisterName(rs1));
                                                    strcat(disassembledInstruction, ", ");
                                                    strcat(disassembledInstruction, getFloatingRegisterName(rs2));
                                                    strcat(disassembledInstruction, ", ");
                                                    strcat(disassembledInstruction, getFloatingRegisterName(rd));
                                                    opcode = NULL;
                                            }
                                            
                                            
                                            switch(opf)
                                            {
                                                case 0b000000001: opcode = "fmovs"; break;
                                                case 0b000000101: opcode = "fnegs"; break;
                                                case 0b000001001: opcode = "fabss"; break;
                                                case 0b011010001: opcode = "fstoi"; break;
                                                case 0b011010010: opcode = "fdtoi"; break;
                                                case 0b011001001: opcode = "fstod"; break;
                                                case 0b011000110: opcode = "fdtos"; break;
                                                case 0b011000100: opcode = "fitos"; break;
                                                case 0b011001000: opcode = "fitod"; break;
                                                case 0b000101001: opcode = "fsqrts"; break;
                                                case 0b000101010: opcode = "fsqrtd"; break;
                                            }
                                            
                                            if(opcode != NULL)
                                            {
                                                    strcpy(disassembledInstruction, opcode);
                                                    strcat(disassembledInstruction, " ");
                                                    strcat(disassembledInstruction, getFloatingRegisterName(rs2));
                                                    strcat(disassembledInstruction, ", ");
                                                    strcat(disassembledInstruction, getFloatingRegisterName(rd));
                                                    opcode = NULL;
                                            }
                                            
                                            break;
                                        }
                                        
                                        
					case 0b110101:  // FPop2
                                        {       
                                            switch(opf)
                                            {
                                                case 0b001010001: opcode = "fcmps"; break;
                                                case 0b001010010: opcode = "fcmpd"; break;
                                                case 0b001010101: opcode = "fcmpes"; break;
                                                case 0b001010110: opcode = "fcmped"; break;
                                            }
                                            
                                            if(opcode != NULL)
                                            {
                                                    strcpy(disassembledInstruction, opcode);
                                                    strcat(disassembledInstruction, " ");
                                                    strcat(disassembledInstruction, getFloatingRegisterName(rs1));
                                                    strcat(disassembledInstruction, ", ");
                                                    strcat(disassembledInstruction, getFloatingRegisterName(rs2));
                                                    opcode = NULL;
                                            }
                                            
                                            break;
                                        }
				}


				switch(op3)
				{
					// B.26. Return from Trap Instruction
					case 0b111001: opcode = "rett"; break;
					
					// B.32. Flush Instruction Memory
					case 0b111011: opcode = "flush"; break;
				}
				
				if(opcode != NULL)
				{
					strcpy(disassembledInstruction, opcode);
					strcat(disassembledInstruction, " ");
					address = getAddress(rs1, rs2, i, simm13, 1);
					strcat(disassembledInstruction, address);
					opcode = NULL;
				}
			}
        }
		
	if(address != NULL)
		free(address);
	if(reg_or_imm != NULL)
		free(reg_or_imm);
	free(hexNumber);
	return disassembledInstruction;
}



/*
 * Translates the effective address for LOAD/STORE instructions
 * and returns the translated address as a string,
 * e.g. '%g3 + 0x25' or '%l2 + %l3'.
 * 
 * The effective address for a load/store instruction is 
 * r[rs1] + r[rs2], if i = 0, or r[rs1] + sign_ext(simm13), if i = 1.
 * The 'i' bit selects the second ALU operand for (integer) arithmetic and
 * load/store instructions. If i = 0, the operand is r[rs2]. If i = 1, 
 * the operand is 'simm13', sign-extended from 13 to 32 bits.
 */
char* getAddress(unsigned long rs1, unsigned long rs2, unsigned long i, unsigned long simm13, int registerTypeIdentifier)
{
	char* address = (char*)malloc(30);
	char* hexNumber = (char*)malloc(32);
	address[0] = '\0';
	

        // Integer, Floating-point or Co-Processor register?
        switch(registerTypeIdentifier)
        {
                case 1: strcat(address, getIntegerRegisterName(rs1)); break;           // Integer register.
                case 2: strcat(address, getFloatingRegisterName(rs1)); break;          // Floating point register.
                case 3: strcat(address, getCoProcessorRegisterName(rs1)); break;       // Co-Processor register.
        }

        // Second operand is rs2.
	if(i == 0) 
	{
            strcat(address, " + ");
            switch(registerTypeIdentifier)
            {
                    case 1: strcat(address, getIntegerRegisterName(rs2)); break;            // Integer register.
                    case 2: strcat(address, getFloatingRegisterName(rs2)); break;           // Floating point register.
                    case 3: strcat(address, getCoProcessorRegisterName(rs2)); break;        // Co-Processor register.
            }

	}
        
        // Second operand is simm13.
	else
	{
		strcat(address, " + ");
		sprintf(hexNumber, "0x%lX", simm13);
		strcat(address, hexNumber);
	}
	
	free(hexNumber);
	return address;
}



/*
 * Returns the second operand of a LOAD/STORE instruction as a string.
 * 
 * The 'i' bit selects the second ALU operand for (integer) arithmetic and
 * load/store instructions. If i = 0, the operand is r[rs2]. If i = 1, 
 * the operand is 'simm13', sign-extended from 13 to 32 bits.
 */
char* getReg_Or_Imm(unsigned long rs2, unsigned long i, unsigned long simm13, int registerTypeIdentifier)
{
	char* address = (char*)malloc(30);
	char* hexNumber = (char*)malloc(32);
	
        // Second operand is rs2.
	if(i == 0) 
	{
                // Integer, Floating-point or Co-Processor register?
		switch(registerTypeIdentifier)
		{
			case 1: strcpy(address, getIntegerRegisterName(rs2)); break;       // Integer register.
			case 2: strcpy(address, getFloatingRegisterName(rs2)); break;      // Floating point register.
			case 3: strcpy(address, getCoProcessorRegisterName(rs2)); break;   // Co-Processor register.
		}
	}
        
        // Second operand is simm13.
	else
	{
		sprintf(hexNumber, "0x%lX", simm13);
		strcpy(address, hexNumber);
	}
	free(hexNumber);
	return address;
}



/*
 * Returns the mnemonic name of an Integer register as string
 * (e.g. %g6, %l7) taking 'registerIdentifier' (e.g. 6, 7) as argument.
 * Translation is done according to the specification given in
 * Chapter - 4 (Registers) of SPARC v8 manual.
 * 
 * in[0] - in[7]         => r[24] - r[31]
 * local[0] - local[7]   => r[16] - r[23]
 * out[0] - out[7]       => r[8] - r[15]
 * global[0] - global[7] => r[0] - r[7]
 */
char* getIntegerRegisterName(unsigned long registerIdentifier)
{
	char* registerName = (char*) malloc(4);
	char* registerIndex = (char*)malloc(2);
	
	if((registerIdentifier >= 0) && (registerIdentifier <=7))
		strcpy(registerName, "%g");
	else
		if((registerIdentifier >= 8) && (registerIdentifier <=15))
			strcpy(registerName, "%o");
		else
			if((registerIdentifier >= 16) && (registerIdentifier <=23))
				strcpy(registerName, "%l");
			else
				strcpy(registerName, "%i");
	
	sprintf(registerIndex, "%ld", (registerIdentifier % 8));
	strcat(registerName, registerIndex);
	
	free(registerIndex);
	return registerName;
}



/*
 * Returns the mnemonic name of a Floating-point register as string
 * (e.g. %f31) taking 'registerIdentifier' (e.g. 31) as argument.
 */
char* getFloatingRegisterName(unsigned long registerIdentifier)
{
	char* registerName = (char*) malloc(4);
	char* registerIndex = (char*)malloc(3);
	
	strcpy(registerName, "%f");
	sprintf(registerIndex, "%ld", registerIdentifier);
	strcat(registerName, registerIndex);
	
	free(registerIndex);
	return registerName;
}



/*
 * Returns the mnemonic name of a Co-Processor register as string
 * (e.g. %f31) taking 'registerIdentifier' (e.g. 31) as argument.
 */
char* getCoProcessorRegisterName(unsigned long registerIdentifier)
{
	char* registerName = (char*) malloc(4);
	char* registerIndex = (char*)malloc(3);
	
	strcpy(registerName, "%f");
	sprintf(registerIndex, "%ld", registerIdentifier);
	strcat(registerName, registerIndex);
	
	free(registerIndex);
	return registerName;
}
