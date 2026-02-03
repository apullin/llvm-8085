# I8085 Optimization Baseline

Generated: 2026-02-03

Method:
- Build examples with `tooling/examples/size_report.sh`.
- `.text/.data/.bss` sizes from `llvm-size -A`.
- Instruction counts from `llvm-objdump -d --no-show-raw-insn`.

| Example | Opt | .text | .data | .bss | Insns |
| --- | --- | --- | --- | --- | --- |
| fib | O1 | 278 | 0 | 0 | 218 |
| fib | O2 | 278 | 0 | 0 | 218 |
| fib | Os | 321 | 0 | 0 | 264 |
| q7_8_matmul | O1 | 125 | 0 | 0 | 113 |
| q7_8_matmul | O2 | 125 | 0 | 0 | 113 |
| q7_8_matmul | Os | 125 | 0 | 0 | 113 |
| opt_sanity | O1 | 91 | 0 | 0 | 89 |
| opt_sanity | O2 | 91 | 0 | 0 | 89 |
| opt_sanity | Os | 91 | 0 | 0 | 89 |
| deep_recursion | O1 | 183 | 0 | 0 | 142 |
| deep_recursion | O2 | 2412 | 0 | 0 | 1517 |
| deep_recursion | Os | 2400 | 0 | 0 | 1511 |
