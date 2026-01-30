

![Intel logo](2dfa6ac3edfe874f68aa0cbccaa42322_img.jpg)

Intel logo

# 8085AH/8085AH-2/8085AH-1 8-BIT HMOS MICROPROCESSORS

- Single +5V Power Supply with 10% Voltage Margins
  - 3 MHz, 5 MHz and 6 MHz Selections Available
  - 20% Lower Power Consumption than 8085A for 3 MHz and 5 MHz
  - 1.3  $\mu$ s Instruction Cycle (8085AH); 0.8  $\mu$ s (8085AH-2); 0.67  $\mu$ s (8085AH-1)
  - 100% Software Compatible with 8080A
  - On-Chip Clock Generator (with External Crystal, LC or RC Network)
- On-Chip System Controller; Advanced Cycle Status Information Available for Large System Control
  - Four Vectored Interrupt Inputs (One Is Non-Maskable) Plus an 8080A-Compatible Interrupt
  - Serial In/Serial Out Port
  - Decimal, Binary and Double Precision Arithmetic
  - Direct Addressing Capability to 64K Bytes of Memory
  - Available in 40-Lead Cerdip and Plastic Packages  
(See Packaging Spec., Order #231369)

The Intel 8085AH is a complete 8-bit parallel Central Processing Unit (CPU) implemented in N-channel, depletion load, silicon gate technology (HMOS). Its instruction set is 100% software compatible with the 8080A microprocessor, and it is designed to improve the present 8080A's performance by higher system speed. Its high level of system integration allows a minimum system of three IC's [8085AH (CPU), 8156H (RAM/IO) and 8755A (EPROM/IO)] while maintaining total system expandability. The 8085AH-2 and 8085AH-1 are faster versions of the 8085AH.

The 8085AH incorporates all of the features that the 8224 (clock generator) and 8228 (system controller) provided for the 8080A, thereby offering a higher level of system integration.

The 8085AH uses a multiplexed data bus. The address is split between the 8-bit address bus and the 8-bit data bus. The on-chip address latches of 8155H/8156H/8755A memory products allow a direct interface with the 8085AH.

![Figure 1. 8085AH CPU Functional Block Diagram](8642df2e3828b25d27362bec6d5a0eae_img.jpg)

The diagram illustrates the internal architecture of the 8085AH CPU. At the top, an 8-bit internal data bus connects various functional blocks. The Interrupt Control block handles external interrupts (INTR, RST 5.5, RST 6.5, RST 7.5) and internal interrupts (INTA, TRAP). The Serial I/O Control block manages serial data (SID, SOD). The central processing unit includes an Accumulator, a Temp Reg, a Flag Flip Flops block, an Arithmetic Logic Unit (ALU), and an Instruction Register. The ALU performs arithmetic and logic operations. The Instruction Decoder and Machine Cycle Encoding block processes instructions. The Register Array contains the B, C, D, E, H, L, Stack Pointer, Program Counter, and Address Latch. The Timing and Control block generates clock signals (CLK OUT, READY) and control signals (RD, WR, ALE, S0, S1, IO/M, HOLD, HLDA, RESET IN, RESET OUT). The Address Buffer (A<sub>0</sub>-A<sub>9</sub>) and Data/Address Buffer (AD<sub>0</sub>-AD<sub>7</sub>) manage the external address and data buses.

Figure 1. 8085AH CPU Functional Block Diagram

Figure 1. 8085AH CPU Functional Block Diagram

![Figure 2. 8085AH Pin Configuration](768b8ab0d55761b205087c079df1e6e6_img.jpg)

The pin configuration diagram shows the 40-pin package of the 8085AH. The pins are numbered 1 to 40. The pin assignments are as follows:

| Pin Number | Pin Name  | Pin Number | Pin Name  |
|------------|-----------|------------|-----------|
| 1          | X1        | 21         | Vss       |
| 2          | X2        | 22         | AD7       |
| 3          | RESET OUT | 23         | AD6       |
| 4          | SOD       | 24         | AD5       |
| 5          | SID       | 25         | AD4       |
| 6          | TRAP      | 26         | AD3       |
| 7          | RST 7.5   | 27         | AD2       |
| 8          | RST 6.5   | 28         | AD1       |
| 9          | RST 5.5   | 29         | AD0       |
| 10         | INTR      | 30         | INTA      |
| 11         | INTA      | 31         | WR        |
| 12         | AD0       | 32         | RD        |
| 13         | AD1       | 33         | S1        |
| 14         | AD2       | 34         | IO/M      |
| 15         | AD3       | 35         | READY     |
| 16         | AD4       | 36         | RESET IN  |
| 17         | AD5       | 37         | CLK (OUT) |
| 18         | AD6       | 38         | HLDA      |
| 19         | AD7       | 39         | HOLD      |
| 20         | Vcc       | 40         | Vcc       |

Figure 2. 8085AH Pin Configuration

Figure 2. 8085AH Pin Configuration

Table 1. Pin Description

| Symbol                                   | Type | Name and Function                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
|------------------------------------------|------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| A <sub>8</sub> –A <sub>15</sub>          | O    | <b>ADDRESS BUS:</b> The most significant 8 bits of memory address or the 8 bits of the I/O address, 3-stated during Hold and Halt modes and during RESET.                                                                                                                                                                                                                                                                                                                                                                                                                          |
| AD <sub>0</sub> –7                       | I/O  | <b>MULTIPLEXED ADDRESS/DATA BUS:</b> Lower 8 bits of the memory address (or I/O address) appear on the bus during the first clock cycle (T state) of a machine cycle. It then becomes the data bus during the second and third clock cycles.                                                                                                                                                                                                                                                                                                                                       |
| ALE                                      | O    | <b>ADDRESS LATCH ENABLE:</b> It occurs during the first clock state of a machine cycle and enables the address to get latched into the on-chip latch of peripherals. The falling edge of ALE is set to guarantee setup and hold times for the address information. The falling edge of ALE can also be used to strobe the status information. ALE is never 3-stated.                                                                                                                                                                                                               |
| S <sub>0</sub> , S <sub>1</sub> and IO/M | O    | <b>MACHINE CYCLE STATUS:</b><br><b>IO/M S<sub>1</sub> S<sub>0</sub> Status</b><br>0 0 1 Memory write<br>0 1 0 Memory read<br>1 0 1 I/O write<br>1 1 0 I/O read<br>0 1 1 Opcode fetch<br>1 1 1 Interrupt Acknowledge<br>* 0 0 Halt<br>* X X Hold<br>* X X Reset<br>* = 3-state (high impedance)<br>X = unspecified<br>S <sub>1</sub> can be used as an advanced R/W status. IO/M, S <sub>0</sub> and S <sub>1</sub> become valid at the beginning of a machine cycle and remain stable throughout the cycle. The falling edge of ALE may be used to latch the state of these lines. |
| RD                                       | O    | <b>READ CONTROL:</b> A low level on RD indicates the selected memory or I/O device is to be read and that the Data Bus is available for the data transfer, 3-stated during Hold and Halt modes and during RESET.                                                                                                                                                                                                                                                                                                                                                                   |
| WR                                       | O    | <b>WRITE CONTROL:</b> A low level on WR indicates the data on the Data Bus is to be written into the selected memory or I/O location. Data is set up at the trailing edge of WR. 3-stated during Hold and Halt modes and during RESET.                                                                                                                                                                                                                                                                                                                                             |
| READY                                    | I    | <b>READY:</b> If READY is high during a read or write cycle, it indicates that the memory or peripheral is ready to send or receive data. If READY is low, the CPU will wait an integral number of clock cycles for READY to go high before completing the read or write cycle. READY must conform to specified setup and hold times.                                                                                                                                                                                                                                              |
| HOLD                                     | I    | <b>HOLD:</b> Indicates that another master is requesting the use of the address and data buses. The CPU, upon receiving the hold request, will relinquish the use of the bus as soon as the completion of the current bus transfer. Internal processing can continue. The processor can regain the bus only after the HOLD is removed. When the HOLD is acknowledged, the Address, Data RD, WR, and IO/M lines are 3-stated.                                                                                                                                                       |
| HLDA                                     | O    | <b>HOLD ACKNOWLEDGE:</b> Indicates that the CPU has received the HOLD request and that it will relinquish the bus in the next clock cycle. HLDA goes low after the Hold request is removed. The CPU takes the bus one half clock cycle after HLDA goes low.                                                                                                                                                                                                                                                                                                                        |
| INTR                                     | I    | <b>INTERRUPT REQUEST:</b> Is used as a general purpose interrupt. It is sampled only during the next to the last clock cycle of an instruction and during Hold and Halt states. If it is active, the Program Counter (PC) will be inhibited from incrementing and an INTA will be issued. During this cycle a RESTART or CALL instruction can be inserted to jump to the interrupt service routine. The INTR is enabled and disabled by software. It is disabled by Reset and immediately after an interrupt is accepted.                                                          |

Table 1. Pin Description (Continued)

| Symbol                        | Type | Name and Function                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|-------------------------------|------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| INTA                          | O    | <b>INTERRUPT ACKNOWLEDGE:</b> Is used instead of (and has the same timing as) <u>RD</u> during the Instruction cycle after an INTR is accepted. It can be used to activate an 8259A Interrupt chip or some other interrupt port.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| RST 5.5<br>RST 6.5<br>RST 7.5 | I    | <b>RESTART INTERRUPTS:</b> These three inputs have the same timing as INTR except they cause an internal RESTART to be automatically inserted.<br>The priority of these interrupt is ordered as shown in Table 2. These interrupts have a higher priority than INTR. In addition, they may be individually masked out using the SIM instruction.                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| TRAP                          | I    | <b>TRAP:</b> Trap interrupt is a non-maskable RESTART interrupt. It is recognized at the same time as INTR or RST 5.5–7.5. It is unaffected by any mask or Interrupt Enable. It has the highest priority of any interrupt. (See Table 2.)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| RESET IN                      | I    | <b>RESET IN:</b> Sets the Program Counter to zero and resets the Interrupt Enable and HLDA flip-flops. The data and address buses and the control lines are 3-stated during RESET and because of the asynchronous nature of RESET, the processor's internal registers and flags may be altered by RESET with unpredictable results. <u>RESET IN</u> is a Schmitt-triggered input, allowing connection to an R-C network for power-on RESET delay (see Figure 3). Upon power-up, <u>RESET IN</u> must remain low for at least 10 ms after minimum $V_{CC}$ has been reached. For proper reset operation after the power-up duration, <u>RESET IN</u> should be kept low a minimum of three clock periods. The CPU is held in the reset condition as long as <u>RESET IN</u> is applied. |
| RESET OUT                     | O    | <b>RESET OUT:</b> Reset Out indicates CPU is being reset. Can be used as a system reset. The signal is synchronized to the processor clock and lasts an integral number of clock periods.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| $X_1, X_2$                    | I    | <b><math>X_1</math> and <math>X_2</math>:</b> Are connected to a crystal, LC, or RC network to drive the internal clock generator. $X_1$ can also be an external clock input from a logic gate. The input frequency is divided by 2 to give the processor's internal operating frequency.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| CLK                           | O    | <b>CLOCK:</b> Clock output for use as a system clock. The period of CLK is twice the $X_1, X_2$ input period.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| SID                           | I    | <b>SERIAL INPUT DATA LINE:</b> The data on this line is loaded into accumulator bit 7 whenever a RIM instruction is executed.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| SOD                           | O    | <b>SERIAL OUTPUT DATA LINE:</b> The output SOD is set or reset as specified by the SIM instruction.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| $V_{CC}$                      |      | <b>POWER:</b> + 5 volt supply.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| $V_{SS}$                      |      | <b>GROUND:</b> Reference.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |

Table 2. Interrupt Priority, Restart Address and Sensitivity

| Name    | Priority | Address Branched to <sup>(1)</sup><br>When Interrupt Occurs | Type Trigger                             |
|---------|----------|-------------------------------------------------------------|------------------------------------------|
| TRAP    | 1        | 24H                                                         | Rising Edge AND High Level until Sampled |
| RST 7.5 | 2        | 3CH                                                         | Rising Edge (Latched)                    |
| RST 6.5 | 3        | 34H                                                         | High Level until Sampled                 |
| RST 5.5 | 4        | 2CH                                                         | High Level until Sampled                 |
| INTR    | 5        | (Note 2)                                                    | High Level until Sampled                 |

## NOTES:

1. The processor pushes the PC on the stack before branching to the indicated address.
2. The address branched to depends on the instruction provided to the CPU when the interrupt is acknowledged.

![Figure 3: Power-On Reset Circuit. The diagram shows a circuit for a power-on reset. A resistor R1 is connected between Vcc and the RESET IN pin. A capacitor C1 is connected between the RESET IN pin and ground. A diode is connected between Vcc and the RESET IN pin, with its cathode towards Vcc. The circuit is labeled 'Typical Power-On Reset RC Values*' and '231718-3'. Below the circuit, it specifies R1 = 75 KΩ and C1 = 1 μF, and notes that values may vary due to power supply ramp up time.](9e6062272bbe3ddbb7c0606721d64cf0_img.jpg)

Figure 3: Power-On Reset Circuit. The diagram shows a circuit for a power-on reset. A resistor R1 is connected between Vcc and the RESET IN pin. A capacitor C1 is connected between the RESET IN pin and ground. A diode is connected between Vcc and the RESET IN pin, with its cathode towards Vcc. The circuit is labeled 'Typical Power-On Reset RC Values\*' and '231718-3'. Below the circuit, it specifies R1 = 75 KΩ and C1 = 1 μF, and notes that values may vary due to power supply ramp up time.

Figure 3. Power-On Reset Circuit

## FUNCTIONAL DESCRIPTION

The 8085AH is a complete 8-bit parallel central processor. It is designed with N-channel, depletion load, silicon gate technology (HMOS), and requires a single +5V supply. Its basic clock speed is 3 MHz (8085AH), 5 MHz (8085AH-2), or 6 MHz (8085AH-1), thus improving on the present 8080A's performance with higher system speed. Also it is designed to fit into a minimum system of three IC's: The CPU (8085AH), a RAM/IO (8156H), and an EPROM/IO chip (8755A).

The 8085AH has twelve addressable 8-bit registers. Four of them can function only as two 16-bit register pairs. Six others can be used interchangeably as 8-bit registers or as 16-bit register pairs. The 8085AH register set is as follows:

| Mnemonic   | Register                                     | Contents                  |
|------------|----------------------------------------------|---------------------------|
| ACC or A   | Accumulator                                  | 8 Bits                    |
| PC         | Program Counter                              | 16-Bit Address            |
| BC, DE, HL | General-Purpose Registers; data pointer (HL) | 8-Bits x 6 or 16 Bits x 3 |
| SP         | Stack Pointer                                | 16-Bit Address            |
| Flags or F | Flag Register                                | 5 Flags (8-Bit Space)     |

The 8085AH uses a multiplexed Data Bus. The address is split between the higher 8-bit Address Bus and the lower 8-bit Address/Data Bus. During the first T state (clock cycle) of a machine cycle the low order address is sent out on the Address/Data bus. These lower 8 bits may be latched externally by the Address Latch Enable signal (ALE). During the rest of the machine cycle the data bus is used for memory or I/O data.

The 8085AH provides  $\overline{RD}$ ,  $\overline{WR}$ ,  $S_0$ ,  $S_1$ , and  $\overline{IO/M}$  signals for bus control. An Interrupt Acknowledge signal ( $\overline{INTA}$ ) is also provided. HOLD and all Interrupts are synchronized with the processor's internal clock. The 8085AH also provides Serial Input Data

(SID) and Serial Output Data (SOD) lines for simple serial interface.

In addition to these features, the 8085AH has three maskable, vector interrupt pins, one nonmaskable TRAP interrupt, and a bus vectored interrupt, INTR.

## INTERRUPT AND SERIAL I/O

The 8085AH has 5 interrupt inputs: INTR, RST 5.5, RST 6.5, RST 7.5, and TRAP. INTR is identical in function to the 8080A INT. Each of the three RESTART inputs, 5.5, 6.5, and 7.5, has a programmable mask. TRAP is also a RESTART interrupt but it is nonmaskable.

The three maskable interrupt cause the internal execution of RESTART (saving the program counter in the stack and branching to the RESTART address) if the interrupts are enabled and if the interrupt mask is not set. The nonmaskable TRAP causes the internal execution of a RESTART vector independent of the state of the interrupt enable or masks. (See Table 2.)

There are two different types of inputs in the restart interrupts. RST 5.5 and RST 6.5 are *high level-sensitive* like INTR (and INT on the 8080) and are recognized with the same timing as INTR. RST 7.5 is *rising edge-sensitive*.

For RST 7.5, only a pulse is required to set an internal flip-flop which generates the internal interrupt request (a normally high level signal with a low going pulse is recommended for highest system noise immunity). The RST 7.5 request flip-flop remains set until the request is serviced. Then it is reset automatically. This flip-flop may also be reset by using the SIM instruction or by issuing a  $\overline{RESET}$  IN to the 8085AH. The RST 7.5 internal flip-flop will be set by a pulse on the RST 7.5 pin even when the RST 7.5 interrupt is masked out.

The status of the three RST interrupt masks can only be affected by the SIM instruction and  $\overline{RESET}$  IN. (See SIM, Chapter 5 of the 8080/8085 User's Manual.)

The interrupts are arranged in a fixed priority that determines which interrupt is to be recognized if more than one is pending as follows: TRAP—highest priority, RST 7.5, RST 6.5, RST 5.5, INTR—lowest priority. This priority scheme does not take into account the priority of a routine that was started by a higher priority interrupt. RST 5.5 can interrupt an RST 7.5 routine if the interrupts are re-enabled before the end of the RST 7.5 routine.

The TRAP interrupt is useful for catastrophic events such as power failure or bus error. The TRAP input is recognized just as any other interrupt but has the

highest priority. It is not affected by any flag or mask. The TRAP input is both *edge and level sensitive*. The TRAP input must go high and remain high until it is acknowledged. It will not be recognized again until it goes low, then high again. This avoids any false triggering due to noise or logic glitches. Figure 4 illustrates the TRAP interrupt request circuitry within the 8085AH. Note that the servicing of any interrupt (TRAP, RST 7.5, RST 6.5, RST 5.5, INTR) disables all future interrupts (except TRAPs) until an EI instruction is executed.

![Figure 4: TRAP and RESET In Circuit. This block diagram shows the internal circuitry for TRAP and RESET signals within the 8085AH. An 'EXTERNAL TRAP INTERRUPT REQUEST' line enters a 'SCHMITT TRIGGER'. The output of the trigger is connected to a 'RESET' input and a 'TRAP' input of a D flip-flop (D F/F). The 'RESET IN' line also enters the Schmitt trigger. The 'TRAP F.F.' (Trap Flag Flip-Flop) output is connected to a 'CLEAR' input of the D flip-flop. The 'INTERNAL TRAP ACKNOWLEDGE' signal is connected to the 'CLEAR' input of the D flip-flop. The 'CLK' input of the D flip-flop is connected to a '+5V' source. The 'Q' output of the D flip-flop is connected to an 'AND' gate. The 'TRAP' input of the AND gate is connected to the output of the Schmitt trigger. The output of the AND gate is the 'TRAP INTERRUPT REQUEST' signal.](aa81b9b80bd1e3d723922b3a033564a2_img.jpg)

Figure 4: TRAP and RESET In Circuit. This block diagram shows the internal circuitry for TRAP and RESET signals within the 8085AH. An 'EXTERNAL TRAP INTERRUPT REQUEST' line enters a 'SCHMITT TRIGGER'. The output of the trigger is connected to a 'RESET' input and a 'TRAP' input of a D flip-flop (D F/F). The 'RESET IN' line also enters the Schmitt trigger. The 'TRAP F.F.' (Trap Flag Flip-Flop) output is connected to a 'CLEAR' input of the D flip-flop. The 'INTERNAL TRAP ACKNOWLEDGE' signal is connected to the 'CLEAR' input of the D flip-flop. The 'CLK' input of the D flip-flop is connected to a '+5V' source. The 'Q' output of the D flip-flop is connected to an 'AND' gate. The 'TRAP' input of the AND gate is connected to the output of the Schmitt trigger. The output of the AND gate is the 'TRAP INTERRUPT REQUEST' signal.

Figure 4. TRAP and RESET In Circuit

The TRAP interrupt is special in that it disables interrupts, but preserves the previous interrupt enable status. Performing the first RIM instruction following a TRAP interrupt allows you to determine whether interrupts were enabled or disabled prior to the TRAP. All subsequent RIM instructions provide current interrupt enable status. Performing a RIM instruction following INTR, or RST 5.5–7.5 will provide current Interrupt Enable status, revealing that interrupts are disabled. See the description of the RIM instruction in the 8080/8085 Family User's Manual.

The serial I/O system is also controlled by the RIM and SIM instruction. SID is read by RIM, and SIM sets the SOD data.

### DRIVING THE X<sub>1</sub> AND X<sub>2</sub> INPUTS

You may drive the clock inputs of the 8085AH, 8085AH-2, or 8085AH-1 with a crystal, an LC tuned circuit, an RC network, or an external clock source. The crystal frequency must be at least 1 MHz, and must be twice the desired internal clock frequency;

hence, the 8085AH is operated with a 6 MHz crystal (for 3 MHz clock), the 8085AH-2 operated with a 10 MHz crystal (for 5 MHz clock), and the 8085AH-1 can be operated with a 12 MHz crystal (for 6 MHz clock). If a crystal is used, it must have the following characteristics:

Parallel resonance at twice the clock frequency desired

$C_L$  (load capacitance)  $\le 30 \text{ pF}$

$C_S$  (Shunt capacitance)  $\le 7 \text{ pF}$

$R_S$  (equivalent shunt resistance)  $\le 75\Omega$

Drive level: 10 mW

Frequency tolerance:  $\pm 0.005\%$  (suggested)

Note the use of the 20 pF capacitor between X<sub>2</sub> and ground. This capacitor is required with crystal frequencies below 4 MHz to assure oscillator startup at the correct frequency. A parallel-resonant LC circuit may be used as the frequency-determining network for the 8085AH, providing that its frequency tolerance of approximately  $\pm 10\%$  is acceptable. The components are chosen from the formula:

$$f = \frac{1}{2\pi\sqrt{L(C_{\text{ext}} + C_{\text{int}})}}$$

To minimize variations in frequency, it is recommended that you choose a value for  $C_{\text{ext}}$  that is at least twice that of  $C_{\text{int}}$ , or 30 pF. The use of an LC circuit is not recommended for frequencies higher than approximately 5 MHz.

An RC circuit may be used as the frequency-determining network for the 8085AH if maintaining a precise clock frequency is of no importance. Variations in the on-chip timing generation can cause a wide variation in frequency when using the RC mode. Its advantage is its low component cost. The driving frequency generated by the circuit shown is approximately 3 MHz. It is not recommended that frequencies greatly higher or lower than this be attempted.

Figure 5 shows the recommended clock driver circuits. Note in d and e that pullup resistors are required to assure that the high level voltage of the input is at least 4V and maximum low level voltage of 0.8V.

For driving frequencies up to and including 6 MHz you may supply the driving signal to X<sub>1</sub> and leave X<sub>2</sub> open-circuited (Figure 5d). If the driving frequency is from 6 MHz to 12 MHz, stability of the clock generator will be improved by driving both X<sub>1</sub> and X<sub>2</sub> with a push-pull source (Figure 5e). To prevent self-oscillation of the 8085AH, be sure that X<sub>2</sub> is not coupled back to X<sub>1</sub> through the driving circuit.

![Figure 5: Clock Driver Circuits. Five sub-diagrams (a-e) show different clock driver configurations for the 8085AH microprocessor. (a) Quartz Crystal Clock Driver: Uses a crystal and 20 pF capacitors. (b) LC Tuned Circuit Clock Driver: Uses an inductor (L_EXT) and capacitor (C_EXT). (c) RC Circuit Clock Driver: Uses a 20 pF capacitor and a 6K resistor. (d) 1-6 MHz Input Frequency Clock Driver Circuit: Uses a NAND gate and a 470Ω resistor. (e) 1-12 MHz Input Frequency External Clock Driver Circuit: Uses a NAND gate, a 470Ω resistor, and an OR gate.](a7d78d22e465dea388b31d0739f9d0cd_img.jpg)

**a. Quartz Crystal Clock Driver** (231718-5)  
 \*20 pF capacitors required for crystal frequency  $\le 4$  MHz only.

**b. LC Tuned Circuit Clock Driver** (231718-6)

**c. RC Circuit Clock Driver** (231718-7)

**d. 1-6 MHz Input Frequency Clock Driver Circuit** (231718-8)  
 \*LOW TIME  $> 60$  ns  
 \* $X_2$  left floating

**e. 1-12 MHz Input Frequency External Clock Driver Circuit** (231718-9)  
 \*LOW TIME  $> 40$  ns

Figure 5: Clock Driver Circuits. Five sub-diagrams (a-e) show different clock driver configurations for the 8085AH microprocessor. (a) Quartz Crystal Clock Driver: Uses a crystal and 20 pF capacitors. (b) LC Tuned Circuit Clock Driver: Uses an inductor (L\_EXT) and capacitor (C\_EXT). (c) RC Circuit Clock Driver: Uses a 20 pF capacitor and a 6K resistor. (d) 1-6 MHz Input Frequency Clock Driver Circuit: Uses a NAND gate and a 470Ω resistor. (e) 1-12 MHz Input Frequency External Clock Driver Circuit: Uses a NAND gate, a 470Ω resistor, and an OR gate.

Figure 5. Clock Driver Circuits

## GENERATING AN 8085AH WAIT STATE

If your system requirements are such that slow memories or peripheral devices are being used, the circuit shown in Figure 6 may be used to insert one WAIT state in each 8085AH machine cycle.

The D flip-flops should be chosen so that

- CLK is rising edge-triggered
- CLEAR is low-level active.

![Figure 6: Generation of a Wait State for 8085AH CPU. A schematic showing two D flip-flops connected in series. The first flip-flop is triggered by ALE' and has a CLEAR input connected to the 8085AH CLK OUTPUT. Its Q output drives the D input of the second flip-flop. The second flip-flop is triggered by the 8085AH CLK OUTPUT and has its Q output connected to the 8085AH READY INPUT. A feedback loop connects the Q output of the second flip-flop back to the CLEAR input of the first flip-flop.](6df5629bc2fc6d82f1a1edf9d7340113_img.jpg)

231718-10  
 \*ALE and CLK (OUT) should be buffered if CLK input of latch exceeds 8085AH IOI or IOH.

Figure 6: Generation of a Wait State for 8085AH CPU. A schematic showing two D flip-flops connected in series. The first flip-flop is triggered by ALE' and has a CLEAR input connected to the 8085AH CLK OUTPUT. Its Q output drives the D input of the second flip-flop. The second flip-flop is triggered by the 8085AH CLK OUTPUT and has its Q output connected to the 8085AH READY INPUT. A feedback loop connects the Q output of the second flip-flop back to the CLEAR input of the first flip-flop.

Figure 6. Generation of a Wait State for 8085AH CPU

As in the 8080, the READY line is used to extend the read and write pulse lengths so that the 8085AH can be used with slow memory. HOLD causes the CPU to relinquish the bus when it is through with it by floating the Address and Data Buses.

## SYSTEM INTERFACE

The 8085AH family includes memory components, which are directly compatible to the 8085AH CPU. For example, a system consisting of the three chips, 8085AH, 8156H and 8755A will have the following features:

- 2K Bytes EPROM
- 256 Bytes RAM
- 1 Timer/Counter
- 4 8-bit I/O Ports
- 1 6-bit I/O Port
- 4 Interrupt Levels
- Serial In/Serial Out Ports

This minimum system, using the standard I/O technique is as shown in Figure 7.

In addition to the standard I/O, the memory mapped I/O offers an efficient I/O addressing technique. With this technique, an area of memory address space is assigned for I/O address, thereby, using the memory address for I/O manipulation. Figure 8

shows the system configuration of Memory Mapped I/O using 8085AH.

The 8085AH CPU can also interface with the standard memory that does *not* have the multiplexed address/data bus. It will require a simple 8-bit latch as shown in Figure 9.

![Figure 7: 8085AH Minimum System (Standard I/O Technique) schematic diagram. The diagram shows the 8085AH CPU connected to an 8156H RAM and an 8755A I/O chip. The CPU's address/data bus (8 lines) is connected to the 8156H's DATA/ADDR bus (8 lines) and the 8755A's DATA/ADDR bus (8 lines). The CPU's control signals (ALE, RD, WR, IO/M, RDY, CLK) are connected to the 8156H and 8755A. The 8156H has 8-bit ports A, B, and C. The 8755A has 8-bit ports A and B, and a 6-bit port C. The 8755A also has an IOR signal connected to its IOR pin. Power supply lines (Vss, Vcc) are shown for all components. A NOTE section at the bottom left indicates that certain connections are optional.](0236eff05bcb8f3a343ea7933aaa306b_img.jpg)

**8085AH**

**8156H**

**8755A**

**NOTE:**  
Optional Connection

23171B-11

Figure 7: 8085AH Minimum System (Standard I/O Technique) schematic diagram. The diagram shows the 8085AH CPU connected to an 8156H RAM and an 8755A I/O chip. The CPU's address/data bus (8 lines) is connected to the 8156H's DATA/ADDR bus (8 lines) and the 8755A's DATA/ADDR bus (8 lines). The CPU's control signals (ALE, RD, WR, IO/M, RDY, CLK) are connected to the 8156H and 8755A. The 8156H has 8-bit ports A, B, and C. The 8755A has 8-bit ports A and B, and a 6-bit port C. The 8755A also has an IOR signal connected to its IOR pin. Power supply lines (Vss, Vcc) are shown for all components. A NOTE section at the bottom left indicates that certain connections are optional.

Figure 7. 8085AH Minimum System (Standard I/O Technique)

Figure 8. 8085 Minimum System (Memory Mapped I/O)

![Block diagram of an 8085AH minimum system with memory-mapped I/O. The 8085AH microprocessor is connected to an 8156H (RAM + I/O + Counter/Timer) and an 8755A (EPROM + I/O). The 8156H has a 6-bit data bus and an 8-bit address/data bus. The 8755A has an 8-bit data bus and an 8-bit address/data bus. Both chips share the 8085AH's address lines A8-A15 and control lines RD, WR, ALE, IO/M, and READY. The 8156H's CE is connected to ALE, and its IO/M is connected to IO/M. The 8755A's CE is connected to RD, and its IO/M is connected to WR. The 8085AH's RESET OUT is connected to the 8156H's RESET. The 8156H's TIMER OUT is connected to the 8085AH's RESET IN. The 8755A's RDY is connected to the 8085AH's READY. Vcc and GND connections are shown for both chips.](35a7554182eb055209552843f341a1ae_img.jpg)

**8085AH**

**8156H [RAM + I/O + COUNTER/TIMER]**

**8755A [EPROM + I/O]**

**8085AH Signals:** A8-15, AD0-7, ALE, RD, WR, IO/M, CLK, RESET OUT, READY

**8156H Signals:** RESET, TIMER IN, WR, RD, ALE, CE, AD 0-7, IO/M

**8755A Signals:** A8-A10, AD 0-7, CE, IO/M, ALE, RD, WR, CLK, RST, RDY

**Connections:**

- 8085AH A8-15 to 8156H A8-15 and 8755A A8-A10
- 8085AH AD0-7 to 8156H AD0-7 and 8755A AD0-7
- 8085AH ALE to 8156H CE and 8755A CE
- 8085AH RD to 8755A RD
- 8085AH WR to 8755A WR
- 8085AH IO/M to 8156H IO/M and 8755A IO/M
- 8085AH CLK to 8156H CLK and 8755A CLK
- 8085AH RESET OUT to 8156H RESET
- 8156H TIMER OUT to 8085AH RESET IN
- 8755A RDY to 8085AH READY

**Power:** Vcc and GND connections for both chips.

**Bus Widths:** 8156H has (6) and (8) bit buses; 8755A has (8) bit buses.

**\*NOTE:** Optional Connection

231718-12

Block diagram of an 8085AH minimum system with memory-mapped I/O. The 8085AH microprocessor is connected to an 8156H (RAM + I/O + Counter/Timer) and an 8755A (EPROM + I/O). The 8156H has a 6-bit data bus and an 8-bit address/data bus. The 8755A has an 8-bit data bus and an 8-bit address/data bus. Both chips share the 8085AH's address lines A8-A15 and control lines RD, WR, ALE, IO/M, and READY. The 8156H's CE is connected to ALE, and its IO/M is connected to IO/M. The 8755A's CE is connected to RD, and its IO/M is connected to WR. The 8085AH's RESET OUT is connected to the 8156H's RESET. The 8156H's TIMER OUT is connected to the 8085AH's RESET IN. The 8755A's RDY is connected to the 8085AH's READY. Vcc and GND connections are shown for both chips.

![Block diagram of an 8085AH microprocessor system using standard memories. The diagram shows the 8085AH chip connected to standard memory and I/O devices. The 8085AH has 40 pins. Pins 1-8 are TRAP, RST7, RST6, RST5, INTR, INTA, ADDR, and ADDR respectively. Pins 9-16 are ADDR/DATA, ALE, RD, WR, IO/M, RDY, CLK, and RESET OUT. Pins 17-24 are X1, X2, RESET IN, HOLD, HLDA, SOD, SID, S1, S0, and RDY. Pins 25-32 are Vss, Vcc, and three ground pins. Pins 33-40 are ADDR, DATA, RD, WR, IO/M, RDY, CLK, and RESET OUT. The ADDR/DATA bus is split into an 8-bit address bus (latched) and an 8-bit data bus. The 8-bit address bus is connected to standard memory and I/O devices. The 8-bit data bus is bidirectional. The RD and WR signals are connected to standard memory and I/O devices. The IO/M signal is connected to standard memory and I/O devices. The RDY signal is connected to standard memory and I/O devices. The CLK signal is connected to standard memory and I/O devices. The RESET OUT signal is connected to standard memory and I/O devices. The standard memory block has IO/M (CS), WR, RD, DATA, and ADDR (CS) signals. The standard I/O block has CLK, RESET, IO/M (CS), WR, RD, DATA, and ADDR signals. The I/O ports and controls are connected to the standard I/O block. The Vcc pins are connected to the standard memory and I/O blocks.](562f471e8153729557e6a4ee6343c32c_img.jpg)

Block diagram of an 8085AH microprocessor system using standard memories. The diagram shows the 8085AH chip connected to standard memory and I/O devices. The 8085AH has 40 pins. Pins 1-8 are TRAP, RST7, RST6, RST5, INTR, INTA, ADDR, and ADDR respectively. Pins 9-16 are ADDR/DATA, ALE, RD, WR, IO/M, RDY, CLK, and RESET OUT. Pins 17-24 are X1, X2, RESET IN, HOLD, HLDA, SOD, SID, S1, S0, and RDY. Pins 25-32 are Vss, Vcc, and three ground pins. Pins 33-40 are ADDR, DATA, RD, WR, IO/M, RDY, CLK, and RESET OUT. The ADDR/DATA bus is split into an 8-bit address bus (latched) and an 8-bit data bus. The 8-bit address bus is connected to standard memory and I/O devices. The 8-bit data bus is bidirectional. The RD and WR signals are connected to standard memory and I/O devices. The IO/M signal is connected to standard memory and I/O devices. The RDY signal is connected to standard memory and I/O devices. The CLK signal is connected to standard memory and I/O devices. The RESET OUT signal is connected to standard memory and I/O devices. The standard memory block has IO/M (CS), WR, RD, DATA, and ADDR (CS) signals. The standard I/O block has CLK, RESET, IO/M (CS), WR, RD, DATA, and ADDR signals. The I/O ports and controls are connected to the standard I/O block. The Vcc pins are connected to the standard memory and I/O blocks.

231718-13

Figure 9. 8085 System (Using Standard Memories)

## **BASIC SYSTEM TIMING**

The 8085AH has a multiplexed Data Bus. ALE is used as a strobe to sample the lower 8-bits of address on the Data Bus. Figure 10 shows an instruction fetch, memory read and I/O write cycle (as would occur during processing of the OUT instruction). Note that during the I/O write and read cycle that the I/O port address is copied on both the upper and lower half of the address.

There are seven possible types of machine cycles. Which of these seven takes place is defined by the status of the three status lines (IO/M, S<sub>1</sub>, S<sub>0</sub>) and

the three control signals ( $\overline{RD}$ ,  $\overline{WR}$ , and  $\overline{INTA}$ ). (See Table 3.) The status lines can be used as advanced controls (for device selection, for example), since they become active at the T<sub>1</sub> state, at the outset of each machine cycle. Control lines  $\overline{RD}$  and  $\overline{WR}$  become active later, at the time when the transfer of data is to take place, so are used as command lines.

A machine cycle normally consists of three T states, with the exception of OPCODE FETCH, which normally has either four or six T states (unless WAIT or HOLD states are forced by the receipt of  $\overline{READY}$  or HOLD inputs). Any T state must be one of ten possible states, shown in Table 4.

**Table 3. 8085AH Machine Cycle Chart**

| Machine Cycle             | Status |    |    | Control         |                 |                   |
|---------------------------|--------|----|----|-----------------|-----------------|-------------------|
|                           | IO/M   | S1 | S0 | $\overline{RD}$ | $\overline{WR}$ | $\overline{INTA}$ |
| OPCODE FETCH (OF)         | 0      | 1  | 1  | 0               | 1               | 1                 |
| MEMORY READ (MR)          | 0      | 1  | 0  | 0               | 1               | 1                 |
| MEMORY WRITE (MW)         | 0      | 0  | 1  | 1               | 0               | 1                 |
| I/O READ (IOR)            | 1      | 1  | 0  | 0               | 1               | 1                 |
| I/O WRITE (IOW)           | 1      | 0  | 1  | 1               | 0               | 1                 |
| ACKNOWLEDGE OF INTR (INA) | 1      | 1  | 1  | 1               | 1               | 0                 |
| BUS IDLE (BI):            | 0      | 1  | 0  | 1               | 1               | 1                 |
| DAD                       | 1      | 1  | 1  | 1               | 1               | 1                 |
| ACK.OF                    | TS     | 0  | 0  | TS              | TS              | 1                 |
| RST,TRAP                  |        |    |    |                 |                 |                   |
| HALT                      |        |    |    |                 |                 |                   |

**Table 4. 8085AH Machine State Chart**

| Machine State      | Status & Buses |      |                                 |                                  | Control                        |                   |     |
|--------------------|----------------|------|---------------------------------|----------------------------------|--------------------------------|-------------------|-----|
|                    | S1,S0          | IO/M | A <sub>8</sub> -A <sub>15</sub> | AD <sub>0</sub> -AD <sub>7</sub> | $\overline{RD}, \overline{WR}$ | $\overline{INTA}$ | ALE |
| T <sub>1</sub>     | X              | X    | X                               | X                                | 1                              | 1                 | 1*  |
| T <sub>2</sub>     | X              | X    | X                               | X                                | X                              | X                 | 0   |
| T <sub>WAIT</sub>  | X              | X    | X                               | X                                | X                              | X                 | 0   |
| T <sub>3</sub>     | X              | X    | X                               | X                                | X                              | X                 | 0   |
| T <sub>4</sub>     | 1              | 0†   | X                               | TS                               | 1                              | 1                 | 0   |
| T <sub>5</sub>     | 1              | 0†   | X                               | TS                               | 1                              | 1                 | 0   |
| T <sub>6</sub>     | 1              | 0†   | X                               | TS                               | 1                              | 1                 | 0   |
| T <sub>RESET</sub> | X              | TS   | TS                              | TS                               | TS                             | 1                 | 0   |
| T <sub>HALT</sub>  | 0              | TS   | TS                              | TS                               | TS                             | 1                 | 0   |
| T <sub>HOLD</sub>  | X              | TS   | TS                              | TS                               | TS                             | 1                 | 0   |

0 = Logic "0"

TS = High Impedance

1 = Logic "1"

X = Unspecified

\*ALE not generated during 2nd and 3rd machine cycles of DAD instruction.

†IO/M = 1 during T<sub>4</sub>-T<sub>6</sub> of INA machine cycle.

![Timing diagram for the 8085AH Basic System Timing, showing CLK, A8-A15, AD0-7, ALE, RD, WR, IO/M, and STATUS signals across three machine cycles M1, M2, and M3.](10c82dcc5f2c237961329dd29d65859c_img.jpg)

The diagram illustrates the timing of the 8085AH microprocessor during a basic system operation, divided into three machine cycles:  $M_1$ ,  $M_2$ , and  $M_3$ . The clock signal (CLK) is shown with time intervals  $T_1$  through  $T_4$  in  $M_1$  and  $T$  in  $M_3$ .

**Address and Data Bus (A<sub>8</sub>-A<sub>15</sub>, AD<sub>0</sub>-<sub>7</sub>):**

- $M_1$ :** A<sub>8</sub>-A<sub>15</sub> carries  $PC_H$  (HIGH ORDER ADDRESS). AD<sub>0</sub>-<sub>7</sub> carries  $PC_L$  (LOW ORDER ADDRESS) followed by DATA FROM MEMORY (INSTRUCTION).
- $M_2$ :** A<sub>8</sub>-A<sub>15</sub> carries  $(PC + 1)_H$ . AD<sub>0</sub>-<sub>7</sub> carries  $(PC + 1)_L$  followed by DATA FROM MEMORY (I/O PORT ADDRESS).
- $M_3$ :** A<sub>8</sub>-A<sub>15</sub> carries IO PORT. AD<sub>0</sub>-<sub>7</sub> carries DATA TO MEMORY OR PERIPHERAL.

**Control Signals:**

- ALE:** Active low during the address setup phase of  $M_1$  and  $M_2$ .
- $\overline{RD}$ :** Active low during  $M_1$  and  $M_2$ .
- WR:** Active low during  $M_3$ .
- IO/M:** Active low during  $M_1$  and  $M_2$ , active high during  $M_3$ .
- STATUS:** Shows  $S_1S_0$  (FETCH) in  $M_1$ , 10 (READ) in  $M_2$ , 01 WRITE in  $M_3$ , and 11 at the end.

231718-14

Timing diagram for the 8085AH Basic System Timing, showing CLK, A8-A15, AD0-7, ALE, RD, WR, IO/M, and STATUS signals across three machine cycles M1, M2, and M3.

**Figure 10. 8085AH Basic System Timing**

### **ABSOLUTE MAXIMUM RATINGS\***

|                                                   |                 |
|---------------------------------------------------|-----------------|
| Ambient Temperature under Bias .....              | 0°C to 70°C     |
| Storage Temperature .....                         | -65°C to +150°C |
| Voltage on Any Pin<br>with Respect to Ground..... | -0.5V to +7V    |
| Power Dissipation.....                            | 1.5W            |

NOTICE: This is a production data sheet. The specifications are subject to change without notice.

**\*WARNING:** Stressing the device beyond the "Absolute Maximum Ratings" may cause permanent damage. These are stress ratings only. Operation beyond the "Operating Conditions" is not recommended and extended exposure beyond the "Operating Conditions" may affect device reliability.

### **D.C. CHARACTERISTICS**

8085AH, 8085AH-2:  $T_A = 0^{\circ}\text{C}$  to  $70^{\circ}\text{C}$ ,  $V_{CC} = 5\text{V} \pm 10\%$ ,  $V_{SS} = 0\text{V}$ ; unless otherwise specified\*

8085AH-1:  $T_A = 0^{\circ}\text{C}$  to  $70^{\circ}\text{C}$ ,  $V_{CC} = 5\text{V} \pm 5\%$ ,  $V_{SS} = 0\text{V}$ ; unless otherwise specified\*

| Symbol    | Parameter               | Min  | Max            | Units         | Test Conditions                       |
|-----------|-------------------------|------|----------------|---------------|---------------------------------------|
| $V_{IL}$  | Input Low Voltage       | -0.5 | +0.8           | V             |                                       |
| $V_{IH}$  | Input High Voltage      | 2.0  | $V_{CC} + 0.5$ | V             |                                       |
| $V_{OL}$  | Output Low Voltage      |      | 0.45           | V             | $I_{OL} = 2\text{mA}$                 |
| $V_{OH}$  | Output High Voltage     | 2.4  |                | V             | $I_{OH} = -400\text{ }\mu\text{A}$    |
| $I_{CC}$  | Power Supply Current    |      | 135            | mA            | 8085AH, 8085AH-2                      |
|           |                         |      | 200            | mA            | 8085AH-1                              |
| $I_{IL}$  | Input Leakage           |      | $\pm 10$       | $\mu\text{A}$ | $0 \le V_{IN} \le V_{CC}$             |
| $I_{LO}$  | Output Leakage          |      | $\pm 10$       | $\mu\text{A}$ | $0.45\text{V} \le V_{OUT} \le V_{CC}$ |
| $V_{ILR}$ | Input Low Level, RESET  | -0.5 | +0.8           | V             |                                       |
| $V_{IHR}$ | Input High Level, RESET | 2.4  | $V_{CC} + 0.5$ | V             |                                       |
| $V_{HY}$  | Hysteresis, RESET       | 0.15 |                | V             |                                       |

### **A.C. CHARACTERISTICS**

8085AH, 8085AH-2:  $T_A = 0^{\circ}\text{C}$  to  $70^{\circ}\text{C}$ ,  $V_{CC} = 5\text{V} \pm 10\%$ ,  $V_{SS} = 0\text{V}$ \*

8085AH-1:  $T_A = 0^{\circ}\text{C}$  to  $70^{\circ}\text{C}$ ,  $V_{CC} = 5\text{V} \pm 5\%$ ,  $V_{SS} = 0\text{V}$

| Symbol     | Parameter                                             | 8085AH (2) |      | 8085AH-2 (2) |      | 8085AH-1 (2) |      | Units |
|------------|-------------------------------------------------------|------------|------|--------------|------|--------------|------|-------|
|            |                                                       | Min        | Max  | Min          | Max  | Min          | Max  |       |
| $t_{CYC}$  | CLK Cycle Period                                      | 320        | 2000 | 200          | 2000 | 167          | 2000 | ns    |
| $t_1$      | CLK Low Time (Standard CLK Loading)                   | 80         |      | 40           |      | 20           |      | ns    |
| $t_2$      | CLK High Time (Standard CLK Loading)                  | 120        |      | 70           |      | 50           |      | ns    |
| $t_r, t_f$ | CLK Rise and Fall Time                                |            | 30   |              | 30   |              | 30   | ns    |
| $t_{XKR}$  | $X_1$ Rising to CLK Rising                            | 20         | 120  | 20           | 100  | 20           | 100  | ns    |
| $t_{XKF}$  | $X_1$ Rising to CLK Falling                           | 20         | 150  | 20           | 110  | 20           | 110  | ns    |
| $t_{AC}$   | $A_8\text{-}15$ Valid to Leading Edge of Control (1)  | 270        |      | 115          |      | 70           |      | ns    |
| $t_{ACL}$  | $A_0\text{-}7$ Valid to Leading Edge of Control       | 240        |      | 115          |      | 60           |      | ns    |
| $t_{AD}$   | $A_0\text{-}15$ Valid to Valid Data In                |            | 575  |              | 350  |              | 225  | ns    |
| $t_{AFR}$  | Address Float after Leading Edge of READ (INTA)       |            | 0    |              | 0    |              | 0    | ns    |
| $t_{AL}$   | $A_8\text{-}15$ Valid before Trailing Edge of ALE (1) | 115        |      | 50           |      | 25           |      | ns    |

#### **\*NOTE:**

For Extended Temperature EXPRESS use M8085AH Electricals Parameters.

### **A.C. CHARACTERISTICS (Continued)**

| Symbol            | Parameter                                                                | 8085AH (2) |     | 8085AH-2 (2) |     | 8085AH-1 (2) |     | Units |
|-------------------|--------------------------------------------------------------------------|------------|-----|--------------|-----|--------------|-----|-------|
|                   |                                                                          | Min        | Max | Min          | Max | Min          | Max |       |
| t <sub>ALL</sub>  | A <sub>0-7</sub> Valid before Trailing Edge of ALE                       | 90         |     | 50           |     | 25           |     | ns    |
| t <sub>ARY</sub>  | READY Valid from Address Valid                                           |            | 220 |              | 100 |              | 40  | ns    |
| t <sub>CA</sub>   | Address (A <sub>8-15</sub> ) Valid after Control                         | 120        |     | 60           |     | 30           |     | ns    |
| t <sub>CC</sub>   | Width of Control Low ( <u>RD</u> , <u>WR</u> , <u>INTA</u> ) Edge of ALE | 400        |     | 230          |     | 150          |     | ns    |
| t <sub>CL</sub>   | Trailing Edge of Control to Leading Edge of ALE                          | 50         |     | 25           |     | 0            |     | ns    |
| t <sub>DW</sub>   | Data Valid to Trailing Edge of <u>WRITE</u>                              | 420        |     | 230          |     | 140          |     | ns    |
| t <sub>HABE</sub> | HLDA to Bus Enable                                                       |            | 210 |              | 150 |              | 150 | ns    |
| t <sub>HABF</sub> | Bus Float after HLDA                                                     |            | 210 |              | 150 |              | 150 | ns    |
| t <sub>HACK</sub> | HLDA Valid to Trailing Edge of CLK                                       | 110        |     | 40           |     | 0            |     | ns    |
| t <sub>HDH</sub>  | HOLD Hold Time                                                           | 0          |     | 0            |     | 0            |     | ns    |
| t <sub>HDS</sub>  | HOLD Setup Time to Trailing Edge of CLK                                  | 170        |     | 120          |     | 120          |     | ns    |
| t <sub>INH</sub>  | INTR Hold Time                                                           | 0          |     | 0            |     | 0            |     | ns    |
| t <sub>INS</sub>  | INTR, RST, and TRAP Setup Time to Falling Edge of CLK                    | 160        |     | 150          |     | 150          |     | ns    |
| t <sub>LA</sub>   | Address Hold Time after ALE                                              | 100        |     | 50           |     | 20           |     | ns    |
| t <sub>LC</sub>   | Trailing Edge of ALE to Leading Edge of Control                          | 130        |     | 60           |     | 25           |     | ns    |
| t <sub>LCK</sub>  | ALE Low During CLK High                                                  | 100        |     | 50           |     | 15           |     | ns    |
| t <sub>LDR</sub>  | ALE to Valid Data during Read                                            |            | 460 |              | 270 |              | 175 | ns    |
| t <sub>LDW</sub>  | ALE to Valid Data during Write                                           |            | 200 |              | 140 |              | 110 | ns    |
| t <sub>LL</sub>   | ALE Width                                                                | 140        |     | 80           |     | 50           |     | ns    |
| t <sub>LRY</sub>  | ALE to READY Stable                                                      |            | 110 |              | 30  |              | 10  | ns    |
| t <sub>RAE</sub>  | Trailing Edge of <u>READ</u> to Re-Enabling of Address                   | 150        |     | 90           |     | 50           |     | ns    |
| t <sub>RD</sub>   | <u>READ</u> (or <u>INTA</u> ) to Valid Data                              |            | 300 |              | 150 |              | 75  | ns    |
| t <sub>RV</sub>   | Control Trailing Edge to Leading Edge of Next Control                    | 400        |     | 220          |     | 160          |     | ns    |
| t <sub>RDH</sub>  | Data Hold Time after <u>READ</u> <u>INTA</u>                             | 0          |     | 0            |     | 0            |     | ns    |
| t <sub>RYH</sub>  | READY Hold Time                                                          | 0          |     | 0            |     | 5            |     | ns    |
| t <sub>RYS</sub>  | READY Setup Time to Leading Edge of CLK                                  | 110        |     | 100          |     | 100          |     | ns    |
| t <sub>WD</sub>   | Data Valid after Trailing Edge of <u>WRITE</u>                           | 100        |     | 60           |     | 30           |     | ns    |
| t <sub>WDL</sub>  | LEADING Edge of <u>WRITE</u> to Data Valid                               |            | 40  |              | 20  |              | 30  | ns    |

#### **NOTES:**

1. A<sub>8</sub>-A<sub>15</sub> address Specs apply IO/M, S<sub>0</sub>, and S<sub>1</sub> except A<sub>8</sub>-A<sub>15</sub> are undefined during T<sub>4</sub>-T<sub>6</sub> of OF cycle whereas IO/M, S<sub>0</sub>, and S<sub>1</sub> are stable.
2. Test Conditions: t<sub>CYC</sub> = 320 ns (8085AH)/200 ns (8085AH-2);/167 ns (8085AH-1); C<sub>L</sub> = 150 pF.
3. For all output timing where C ≠ 150 pF use the following correction factors:
  - 25 pF ≤ C<sub>L</sub> < 150 pF: -0.10 ns/pF
  - 150 pF < C<sub>L</sub> ≤ 300 pF: +0.30 ns/pF
4. Output timings are measured with purely capacitive load.
5. To calculate timing specifications at other values of t<sub>CYC</sub> use Table 5.

### A.C. TESTING INPUT, OUTPUT WAVEFORM

![Timing diagram showing input and output waveforms. The input waveform transitions from 0.45V to 2.4V. The output waveform transitions from 2.4V to 0.45V. Test points are marked at 2.0V and 0.8V on the output transition. The diagram is labeled 231718-15.](0332672e127cd13bb6d2fc8d1e27bfa2_img.jpg)

INPUT/OUTPUT

231718-15

A.C. Testing: Inputs are driven at 2.4V for a Logic "1" and 0.45V for a Logic "0". Timing measurements are made at 2.0V for a Logic "1" and 0.8V for a Logic "0".

Timing diagram showing input and output waveforms. The input waveform transitions from 0.45V to 2.4V. The output waveform transitions from 2.4V to 0.45V. Test points are marked at 2.0V and 0.8V on the output transition. The diagram is labeled 231718-15.

### A.C. TESTING LOAD CIRCUIT

![Circuit diagram showing a Device Under Test (DUT) connected to a load capacitor CL = 150 pF to ground. The diagram is labeled 231718-16.](a26e142d3df5bef41a84a9dd099d7825_img.jpg)

231718-16

$C_L = 100 \text{ pF}$   
 $C_L$  Includes Jig Capacitance

Circuit diagram showing a Device Under Test (DUT) connected to a load capacitor CL = 150 pF to ground. The diagram is labeled 231718-16.

Table 5. Bus Timing Specification as a  $T_{CYC}$  Dependent

| Symbol     | 8085AH             | 8085AH-2           | 8085AH-1           |         |
|------------|--------------------|--------------------|--------------------|---------|
| $t_{AL}$   | $(1/2)T - 45$      | $(1/2)T - 50$      | $(1/2)T - 58$      | Minimum |
| $t_{LA}$   | $(1/2)T - 60$      | $(1/2)T - 50$      | $(1/2)T - 63$      | Minimum |
| $t_{LL}$   | $(1/2)T - 20$      | $(1/2)T - 20$      | $(1/2)T - 33$      | Minimum |
| $t_{LCK}$  | $(1/2)T - 60$      | $(1/2)T - 50$      | $(1/2)T - 68$      | Minimum |
| $t_{LC}$   | $(1/2)T - 30$      | $(1/2)T - 40$      | $(1/2)T - 58$      | Minimum |
| $t_{AD}$   | $(5/2 + N)T - 225$ | $(5/2 + N)T - 150$ | $(5/2 + N)T - 192$ | Maximum |
| $t_{RD}$   | $(3/2 + N)T - 180$ | $(3/2 + N)T - 150$ | $(3/2 + N)T - 175$ | Maximum |
| $t_{RAE}$  | $(1/2)T - 10$      | $(1/2)T - 10$      | $(1/2)T - 33$      | Minimum |
| $t_{CA}$   | $(1/2)T - 40$      | $(1/2)T - 40$      | $(1/2)T - 53$      | Minimum |
| $t_{DW}$   | $(3/2 + N)T - 60$  | $(3/2 + N)T - 70$  | $(3/2 + N)T - 110$ | Minimum |
| $t_{WD}$   | $(1/2)T - 60$      | $(1/2)T - 40$      | $(1/2)T - 53$      | Minimum |
| $t_{CC}$   | $(3/2 + N)T - 80$  | $(3/2 + N)T - 70$  | $(3/2 + N)T - 100$ | Minimum |
| $t_{CL}$   | $(1/2)T - 110$     | $(1/2)T - 75$      | $(1/2)T - 83$      | Minimum |
| $t_{ARY}$  | $(3/2)T - 260$     | $(3/2)T - 200$     | $(3/2)T - 210$     | Maximum |
| $t_{HACK}$ | $(1/2)T - 50$      | $(1/2)T - 60$      | $(1/2)T - 83$      | Minimum |
| $t_{HABF}$ | $(1/2)T + 50$      | $(1/2)T + 50$      | $(1/2)T + 67$      | Maximum |
| $t_{HABE}$ | $(1/2)T + 50$      | $(1/2)T + 50$      | $(1/2)T + 67$      | Maximum |
| $t_{AC}$   | $(2/2)T - 50$      | $(2/2)T - 85$      | $(2/2)T - 97$      | Minimum |
| $t_1$      | $(1/2)T - 80$      | $(1/2)T - 60$      | $(1/2)T - 63$      | Minimum |
| $t_2$      | $(1/2)T - 40$      | $(1/2)T - 30$      | $(1/2)T - 33$      | Minimum |
| $t_{RV}$   | $(3/2)T - 80$      | $(3/2)T - 80$      | $(3/2)T - 90$      | Minimum |
| $t_{LDR}$  | $(4/2 + N)T - 180$ | $(4/2)T - 130$     | $(4/2)T - 159$     | Maximum |

#### NOTE:

N is equal to the total WAIT states.  $T = t_{CYC}$ .

### **WAVEFORMS**

#### **CLOCK**

![Clock waveform diagram showing X1 input and CLK output signals with timing parameters tXKR, tXKF, t1, t2, t3, tLCK, and tCYC.](fe753d01ad5fe6cf150018c958173c6d_img.jpg)

The diagram illustrates the clock waveform generation. The **X1 INPUT** is a square wave. The **CLK OUTPUT** is a square wave derived from the X1 input. Timing parameters are defined as follows:

- $t_{XKR}$ : X1 input to CLK output rise time.
- $t_{XKF}$ : X1 input to CLK output fall time.
- $t_1$ : CLK high period.
- $t_2$ : CLK low period.
- $t_3$ : CLK cycle time.
- $t_{LCK}$ : CLK period.
- $t_{CYC}$ : CLK cycle time.

Reference number: 231718-17

Clock waveform diagram showing X1 input and CLK output signals with timing parameters tXKR, tXKF, t1, t2, t3, tLCK, and tCYC.

#### **READ**

![Read cycle waveform diagram showing CLK, ADDRESS, DATA IN, ALE, and RD/INTA signals with timing parameters tLCK, tCA, tAD, tRAE, tLL, tLA, tAFR, tLDR, tRD, tCC, tCL, tAL, tLC, and tAC.](239211fa511b4ffa685b54b5132ec927_img.jpg)

The diagram shows the timing for a read cycle, divided into three clock periods:  $T_1$ ,  $T_2$ , and  $T_3$ .

Timing parameters for the read cycle include:

- $t_{LCK}$ : CLK period.
- $t_{CA}$ : Address to data setup time.
- $t_{AD}$ : Address to data hold time.
- $t_{RAE}$ : Address to data enable time.
- $t_{LL}$ : Low level time of ALE.
- $t_{LA}$ : Low level time of ALE.
- $t_{AFR}$ : Address to data release time.
- $t_{LDR}$ : Low level time of RD/INTA.
- $t_{RD}$ : RD/INTA high level time.
- $t_{CC}$ : Clock cycle time.
- $t_{CL}$ : Clock low level time.
- $t_{AL}$ : Address low level time.
- $t_{LC}$ : Address low level time.
- $t_{AC}$ : Address cycle time.

Reference number: 231718-18

Read cycle waveform diagram showing CLK, ADDRESS, DATA IN, ALE, and RD/INTA signals with timing parameters tLCK, tCA, tAD, tRAE, tLL, tLA, tAFR, tLDR, tRD, tCC, tCL, tAL, tLC, and tAC.

#### **WRITE**

![Write cycle waveform diagram showing CLK, ADDRESS, DATA OUT, ALE, and WR signals with timing parameters tLCK, tCA, tLOW, tDW, tWD, tWDL, tCC, tCL, tAL, tLC, and tAC.](4a8166688ed7276efb173f550ba47eb4_img.jpg)

The diagram shows the timing for a write cycle, divided into three clock periods:  $T_1$ ,  $T_2$ , and  $T_3$ .

Timing parameters for the write cycle include:

- $t_{LCK}$ : CLK period.
- $t_{CA}$ : Address to data setup time.
- $t_{LOW}$ : Address low level time.
- $t_{DW}$ : Data write time.
- $t_{WD}$ : Data write hold time.
- $t_{WDL}$ : Data write low level time.
- $t_{CC}$ : Clock cycle time.
- $t_{CL}$ : Clock low level time.
- $t_{AL}$ : Address low level time.
- $t_{LC}$ : Address low level time.
- $t_{AC}$ : Address cycle time.

Reference number: 231718-19

Write cycle waveform diagram showing CLK, ADDRESS, DATA OUT, ALE, and WR signals with timing parameters tLCK, tCA, tLOW, tDW, tWD, tWDL, tCC, tCL, tAL, tLC, and tAC.

### WAVEFORMS (Continued)

#### HOLD

![Timing diagram for the HOLD operation showing CLK, HOLD, HLDA, and BUS signals.](e95f47f7a4c01c8889d6d46919b4c73d_img.jpg)

The diagram illustrates the timing for a HOLD operation. The CLK signal has periods T<sub>2</sub>, T<sub>3</sub>, T<sub>HOLD</sub>, and T<sub>1</sub>. The HOLD signal is active during T<sub>2</sub> and T<sub>3</sub>. The HLDA signal is active during T<sub>2</sub> and T<sub>3</sub>. The BUS (ADDRESS, CONTROLS) is active during T<sub>2</sub> and T<sub>3</sub>. Timing parameters include t<sub>HDS</sub> (Hold Data Setup), t<sub>HDM</sub> (Hold Data Hold), t<sub>HACK</sub> (Hold Acknowledge Setup), t<sub>HABF</sub> (Hold Acknowledge Hold), and t<sub>HABE</sub> (Hold Acknowledge End).

231718-20

Timing diagram for the HOLD operation showing CLK, HOLD, HLDA, and BUS signals.

#### READ OPERATION WITH WAIT CYCLE (TYPICAL)—SAME READY TIMING APPLIES TO WRITE

![Timing diagram for a Read operation with a Wait cycle, showing CLK, ADDRESS, DATA IN, ALE, RD/INTA, and READY signals.](011fecb4a85637472f0c697a6cbdb15d_img.jpg)

The diagram shows a read cycle with a wait state. The CLK signal has periods T<sub>1</sub>, T<sub>2</sub>, T<sub>WAIT</sub>, T<sub>3</sub>, and T<sub>1</sub>. The ADDRESS bus (A<sub>8</sub>-A<sub>15</sub> and AD<sub>0</sub>-AD<sub>7</sub>) is active during T<sub>1</sub> and T<sub>2</sub>. The DATA IN bus is active during T<sub>2</sub> and T<sub>3</sub>. The ALE signal is active during T<sub>1</sub>. The RD/INTA signal is active during T<sub>1</sub> and T<sub>2</sub>. The READY signal is active during T<sub>1</sub> and T<sub>2</sub>, and remains active during the T<sub>WAIT</sub> period. Timing parameters include t<sub>LCK</sub>, t<sub>AD</sub>, t<sub>AD</sub>, t<sub>LD</sub>, t<sub>RD</sub>, t<sub>CC</sub>, t<sub>CA</sub>, t<sub>RAE</sub>, t<sub>CL</sub>, t<sub>LL</sub>, t<sub>LA</sub>, t<sub>AFR</sub>, t<sub>AL</sub>, t<sub>AC</sub>, t<sub>LC</sub>, t<sub>LRY</sub>, t<sub>ARY</sub>, t<sub>RYS</sub>, and t<sub>RYH</sub>.

231718-21

Timing diagram for a Read operation with a Wait cycle, showing CLK, ADDRESS, DATA IN, ALE, RD/INTA, and READY signals.

##### NOTE:

1. Ready must remain stable during setup and hold times.

### **WAVEFORMS (Continued)**

#### **INTERRUPT AND HOLD**

![Timing diagram for Interrupt and Hold operations on the 8085 microprocessor. The diagram shows the state of various signals over time, divided into clock cycles T1 through T6, a HOLD cycle, and a subsequent T1-T2 cycle. Signals include A8-15, AD0-7, ALE, <span style='text-decoration: overline;'>RD</span>, <span style='text-decoration: overline;'>INTA</span>, INTR, HOLD, and HLDA. Key events are marked with labels like t<sub>INS</sub>, t<sub>INH</sub>, t<sub>HDS</sub>, t<sub>HDH</sub>, t<sub>HACK</sub>, t<sub>HABF</sub>, and t<sub>HABE</sub>. A 'CALL INST.' is shown on the AD0-7 bus during T3. A 'BUS FLOATING*' period is indicated during T4-T6 and THOLD. A note at the bottom states that IO/<span style='text-decoration: overline;'>M</span> is also floating during this time.](fdcfba1180dc160c7d539c5fb2a6c1e6_img.jpg)

231718-22

**\*NOTE:**  
IO/M is also floating during this time.

Timing diagram for Interrupt and Hold operations on the 8085 microprocessor. The diagram shows the state of various signals over time, divided into clock cycles T1 through T6, a HOLD cycle, and a subsequent T1-T2 cycle. Signals include A8-15, AD0-7, ALE, RD, INTA, INTR, HOLD, and HLDA. Key events are marked with labels like t<sub>INS</sub>, t<sub>INH</sub>, t<sub>HDS</sub>, t<sub>HDH</sub>, t<sub>HACK</sub>, t<sub>HABF</sub>, and t<sub>HABE</sub>. A 'CALL INST.' is shown on the AD0-7 bus during T3. A 'BUS FLOATING\*' period is indicated during T4-T6 and THOLD. A note at the bottom states that IO/M is also floating during this time.

Table 6. Instruction Set Summary

| Mnemonic                    | Instruction Code<br>D <sub>7</sub> D <sub>6</sub> D <sub>5</sub> D <sub>4</sub> D <sub>3</sub> D <sub>2</sub> D <sub>1</sub> D <sub>0</sub> | Operations Description             |
|-----------------------------|---------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------|
| <b>MOVE, LOAD AND STORE</b> |                                                                                                                                             |                                    |
| MOV r1 r2                   | 0 1 D D D S S S                                                                                                                             | Move register to register          |
| MOV M.r                     | 0 1 1 1 0 S S S                                                                                                                             | Move register to memory            |
| MOV r.M                     | 0 1 D D D 1 1 0                                                                                                                             | Move memory to register            |
| MVI r                       | 0 0 D D D 1 1 0                                                                                                                             | Move immediate register            |
| MVI M                       | 0 0 1 1 0 1 1 0                                                                                                                             | Move immediate memory              |
| LXI B                       | 0 0 0 0 0 0 0 1                                                                                                                             | Load immediate register Pair B & C |
| LXI D                       | 0 0 0 1 0 0 0 1                                                                                                                             | Load immediate register Pair D & E |
| LXI H                       | 0 0 1 0 0 0 0 1                                                                                                                             | Load immediate register Pair H & L |
| STAX B                      | 0 0 0 0 0 0 1 0                                                                                                                             | Store A indirect                   |
| STAX D                      | 0 0 0 1 0 0 1 0                                                                                                                             | Store A indirect                   |
| LDAX B                      | 0 0 1 0 1 0 1 0                                                                                                                             | Load A indirect                    |
| LDAX D                      | 0 0 0 1 1 0 1 0                                                                                                                             | Load A indirect                    |
| STA                         | 0 0 1 1 0 0 1 0                                                                                                                             | Store A direct                     |
| LDA                         | 0 0 1 1 1 0 1 0                                                                                                                             | Load A direct                      |
| SHLD                        | 0 0 1 0 0 0 1 0                                                                                                                             | Store H & L direct                 |
| LHLD                        | 0 0 1 0 1 0 1 0                                                                                                                             | Load H & L direct                  |
| XCHG                        | 1 1 1 0 1 0 1 1                                                                                                                             | Exchange D & E, H & L Registers    |
| <b>STACK OPS</b>            |                                                                                                                                             |                                    |
| PUSH B                      | 1 1 0 0 0 1 0 1                                                                                                                             | Push register Pair B & C on stack  |
| PUSH D                      | 1 1 0 1 0 1 0 1                                                                                                                             | Push register Pair D & E on stack  |
| PUSH H                      | 1 1 1 0 0 1 0 1                                                                                                                             | Push register Pair H & L on stack  |
| PUSH PSW                    | 1 1 1 1 0 1 0 1                                                                                                                             | Push A and Flags on stack          |
| POP B                       | 1 1 0 0 0 0 0 1                                                                                                                             | Pop register Pair B & C off stack  |
| POP D                       | 1 1 0 1 0 0 0 1                                                                                                                             | Pop register Pair D & E off stack  |
| POP H                       | 1 1 1 0 0 0 0 1                                                                                                                             | Pop register Pair H & L off stack  |

| Mnemonic                     | Instruction Code<br>D <sub>7</sub> D <sub>6</sub> D <sub>5</sub> D <sub>4</sub> D <sub>3</sub> D <sub>2</sub> D <sub>1</sub> D <sub>0</sub> | Operations Description       |
|------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------|------------------------------|
| <b>STACK OPS (Continued)</b> |                                                                                                                                             |                              |
| POP PSW                      | 1 1 1 1 0 0 0 1                                                                                                                             | Pop A and Flags off stack    |
| XTHL                         | 1 1 1 0 0 0 1 1                                                                                                                             | Exchange top of stack, H & L |
| SPHL                         | 1 1 1 1 1 0 0 1                                                                                                                             | H & L to stack pointer       |
| LXI SP                       | 0 0 1 1 0 0 0 1                                                                                                                             | Load immediate stack pointer |
| INX SP                       | 0 0 1 1 0 0 1 1                                                                                                                             | Increment stack pointer      |
| DCX SP                       | 0 0 1 1 1 0 1 1                                                                                                                             | Decrement stack pointer      |
| <b>JUMP</b>                  |                                                                                                                                             |                              |
| JMP                          | 1 1 0 0 0 0 1 1                                                                                                                             | Jump unconditional           |
| JC                           | 1 1 0 1 1 0 1 0                                                                                                                             | Jump on carry                |
| JNC                          | 1 1 0 1 0 0 1 0                                                                                                                             | Jump on no carry             |
| JZ                           | 1 1 0 0 1 0 1 0                                                                                                                             | Jump on zero                 |
| JNZ                          | 1 1 0 0 0 0 1 0                                                                                                                             | Jump on no zero              |
| JP                           | 1 1 1 1 0 0 1 0                                                                                                                             | Jump on positive             |
| JM                           | 1 1 1 1 1 0 1 0                                                                                                                             | Jump on minus                |
| JPE                          | 1 1 1 0 1 0 1 0                                                                                                                             | Jump on parity even          |
| JPO                          | 1 1 1 0 0 0 1 0                                                                                                                             | Jump on parity odd           |
| PCHL                         | 1 1 1 0 1 0 0 1                                                                                                                             | H & L to program counter     |
| <b>CALL</b>                  |                                                                                                                                             |                              |
| CALL                         | 1 1 0 0 1 1 0 1                                                                                                                             | Call unconditional           |
| CC                           | 1 1 0 1 1 1 0 0                                                                                                                             | Call on carry                |
| CNC                          | 1 1 0 1 0 1 0 0                                                                                                                             | Call on no carry             |
| CZ                           | 1 1 0 0 1 1 0 0                                                                                                                             | Call on zero                 |
| CNZ                          | 1 1 0 0 0 1 0 0                                                                                                                             | Call on no zero              |
| CP                           | 1 1 1 1 0 1 0 0                                                                                                                             | Call on positive             |
| CM                           | 1 1 1 1 1 1 0 0                                                                                                                             | Call on minus                |
| CPE                          | 1 1 1 0 1 1 0 0                                                                                                                             | Call on parity even          |
| CPO                          | 1 1 1 0 0 1 0 0                                                                                                                             | Call on parity odd           |
| <b>RETURN</b>                |                                                                                                                                             |                              |
| RET                          | 1 1 0 0 1 0 0 1                                                                                                                             | Return                       |
| RC                           | 1 1 0 1 1 0 0 0                                                                                                                             | Return on carry              |
| RNC                          | 1 1 0 1 0 0 0 0                                                                                                                             | Return on no carry           |
| RZ                           | 1 1 0 0 1 0 0 0                                                                                                                             | Return on zero               |

Table 6. Instruction Set Summary (Continued)

| Mnemonic                       | Instruction Code<br>D <sub>7</sub> D <sub>6</sub> D <sub>5</sub> D <sub>4</sub> D <sub>3</sub> D <sub>2</sub> D <sub>1</sub> D <sub>0</sub> | Operations<br>Description     |
|--------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------|
| <b>RETURN (Continued)</b>      |                                                                                                                                             |                               |
| RNZ                            | 1 1 0 0 0 0 0 0                                                                                                                             | Return on no zero             |
| RP                             | 1 1 1 1 0 0 0 0                                                                                                                             | Return on positive            |
| RM                             | 1 1 1 1 1 0 0 0                                                                                                                             | Return on minus               |
| RPE                            | 1 1 1 0 1 0 0 0                                                                                                                             | Return on parity even         |
| RPO                            | 1 1 1 0 0 0 0 0                                                                                                                             | Return on parity odd          |
| <b>RESTART</b>                 |                                                                                                                                             |                               |
| RST                            | 1 1 A A A 1 1 1                                                                                                                             | Restart                       |
| <b>INPUT/OUTPUT</b>            |                                                                                                                                             |                               |
| IN                             | 1 1 0 1 1 0 1 1                                                                                                                             | Input                         |
| OUT                            | 1 1 0 1 0 0 1 1                                                                                                                             | Output                        |
| <b>INCREMENT AND DECREMENT</b> |                                                                                                                                             |                               |
| INR r                          | 0 0 D D D 1 0 0                                                                                                                             | Increment register            |
| DCR r                          | 0 0 D D D 1 0 1                                                                                                                             | Decrement register            |
| INR M                          | 0 0 1 1 0 1 0 0                                                                                                                             | Increment memory              |
| DCR M                          | 0 0 1 1 0 1 0 1                                                                                                                             | Decrement memory              |
| INX B                          | 0 0 0 0 0 0 1 1                                                                                                                             | Increment B & C registers     |
| INX D                          | 0 0 0 1 0 0 1 1                                                                                                                             | Increment D & E registers     |
| INX H                          | 0 0 1 0 0 0 1 1                                                                                                                             | Increment H & L registers     |
| DCX B                          | 0 0 0 0 1 0 1 1                                                                                                                             | Decrement B & C               |
| DCX D                          | 0 0 0 1 1 0 1 1                                                                                                                             | Decrement D & E               |
| DCX H                          | 0 0 1 0 1 0 1 1                                                                                                                             | Decrement H & L               |
| <b>ADD</b>                     |                                                                                                                                             |                               |
| ADD r                          | 1 0 0 0 0 S S S                                                                                                                             | Add register to A             |
| ADC r                          | 1 0 0 0 1 S S S                                                                                                                             | Add register to A with carry  |
| ADD M                          | 1 0 C 0 0 1 1 0                                                                                                                             | Add memory to A               |
| ADC M                          | 1 0 0 0 1 1 1 0                                                                                                                             | Add memory to A with carry    |
| ADI                            | 1 1 0 0 0 1 1 0                                                                                                                             | Add immediate to A            |
| ACI                            | 1 1 0 0 1 1 1 0                                                                                                                             | Add immediate to A with carry |
| DAD B                          | 0 0 0 0 1 0 0 1                                                                                                                             | Add B & C to H & L            |

| Mnemonic               | Instruction Code<br>D <sub>7</sub> D <sub>6</sub> D <sub>5</sub> D <sub>4</sub> D <sub>3</sub> D <sub>2</sub> D <sub>1</sub> D <sub>0</sub> | Operations<br>Description             |
|------------------------|---------------------------------------------------------------------------------------------------------------------------------------------|---------------------------------------|
| <b>ADD (Continued)</b> |                                                                                                                                             |                                       |
| DAD D                  | 0 0 0 1 1 0 0 1                                                                                                                             | Add D & E to H & L                    |
| DAD H                  | 0 0 1 0 1 0 0 1                                                                                                                             | Add H & L to H & L                    |
| DAD SP                 | 0 0 1 1 1 0 0 1                                                                                                                             | Add stack pointer to H & L            |
| <b>SUBTRACT</b>        |                                                                                                                                             |                                       |
| SUB r                  | 1 0 0 1 0 S S S                                                                                                                             | Subtract register from A              |
| SBB r                  | 1 0 0 1 1 S S S                                                                                                                             | Subtract register from A with borrow  |
| SUB M                  | 1 0 0 1 0 1 1 0                                                                                                                             | Subtract memory from A                |
| SBB M                  | 1 0 0 1 1 1 1 0                                                                                                                             | Subtract memory from A with borrow    |
| SUI                    | 1 1 0 1 0 1 1 0                                                                                                                             | Subtract immediate from A             |
| SBI                    | 1 1 0 1 1 1 1 0                                                                                                                             | Subtract immediate from A with borrow |
| <b>LOGICAL</b>         |                                                                                                                                             |                                       |
| ANA r                  | 1 0 1 0 0 S S S                                                                                                                             | And register with A                   |
| XRA r                  | 1 0 1 0 1 S S S                                                                                                                             | Exclusive OR register with A          |
| ORA r                  | 1 0 1 1 0 S S S                                                                                                                             | OR register with A                    |
| CMP r                  | 1 0 1 1 1 S S S                                                                                                                             | Compare register with A               |
| ANA M                  | 1 0 1 0 0 1 1 0                                                                                                                             | And memory with A                     |
| XRA M                  | 1 0 1 0 1 1 1 0                                                                                                                             | Exclusive OR memory with A            |
| ORA M                  | 1 0 1 1 0 1 1 0                                                                                                                             | OR memory with A                      |
| CMP M                  | 1 0 1 1 1 1 1 0                                                                                                                             | Compare memory with A                 |
| ANI                    | 1 1 1 0 0 1 1 0                                                                                                                             | And immediate with A                  |
| XRI                    | 1 1 1 0 1 1 1 0                                                                                                                             | Exclusive OR immediate with A         |
| ORI                    | 1 1 1 1 0 1 1 0                                                                                                                             | OR immediate with A                   |
| CPI                    | 1 1 1 1 1 1 1 0                                                                                                                             | Compare immediate with A              |

Table 6. Instruction Set Summary (Continued)

| Mnemonic        | Instruction Code                                                                                                        | Operations Description       |
|-----------------|-------------------------------------------------------------------------------------------------------------------------|------------------------------|
|                 | D <sub>7</sub> D <sub>6</sub> D <sub>5</sub> D <sub>4</sub> D <sub>3</sub> D <sub>2</sub> D <sub>1</sub> D <sub>0</sub> |                              |
| <b>ROTATE</b>   |                                                                                                                         |                              |
| RLC             | 0 0 0 0 0 1 1 1                                                                                                         | Rotate A left                |
| RRC             | 0 0 0 0 1 1 1 1                                                                                                         | Rotate A right               |
| RAL             | 0 0 0 1 0 1 1 1                                                                                                         | Rotate A left through carry  |
| RAR             | 0 0 0 1 1 1 1 1                                                                                                         | Rotate A right through carry |
| <b>SPECIALS</b> |                                                                                                                         |                              |
| CMA             | 0 0 1 0 1 1 1 1                                                                                                         | Complement A                 |
| STC             | 0 0 1 1 0 1 1 1                                                                                                         | Set carry                    |
| CMC             | 0 0 1 1 1 1 1 1                                                                                                         | Complement carry             |
| DAA             | 0 0 1 0 0 1 1 1                                                                                                         | Decimal adjust A             |

| Mnemonic                       | Instruction Code                                                                                                        | Operations Description |
|--------------------------------|-------------------------------------------------------------------------------------------------------------------------|------------------------|
|                                | D <sub>7</sub> D <sub>6</sub> D <sub>5</sub> D <sub>4</sub> D <sub>3</sub> D <sub>2</sub> D <sub>1</sub> D <sub>0</sub> |                        |
| <b>CONTROL</b>                 |                                                                                                                         |                        |
| EI                             | 1 1 1 1 1 0 1 1                                                                                                         | Enable Interrupts      |
| DI                             | 1 1 1 1 0 0 1 1                                                                                                         | Disable Interrupt      |
| NOP                            | 0 0 0 0 0 0 0 0                                                                                                         | No-operation           |
| HLT                            | 0 1 1 1 0 1 1 0                                                                                                         | Halt                   |
| <b>NEW 8085AH INSTRUCTIONS</b> |                                                                                                                         |                        |
| RIM                            | 0 0 1 0 0 0 0 0                                                                                                         | Read Interrupt Mask    |
| SIM                            | 0 0 1 1 0 0 0 0                                                                                                         | Set Interrupt Mask     |

#### **NOTES:**

1. DDS or SSS: B 000, C 001, D 010, E011, H 100, L101, Memory 110, A 111.
2. Two possible cycle times (6/12) indicate instruction cycles dependent on condition flags.

\*All mnemonics copyrighted © Intel Corporation 1976.