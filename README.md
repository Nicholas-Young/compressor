An attempt at a simple implementation of the LZW compression scheme

Required arguments:
--compress (-c) | --decompress (-d)
--input (-i) <input file name>
--output (-o) <output file name>

Optional arguments:
--min_reps <minimum symbol replications>
--min_sym_len <minimum symbol length>
--max_sym_len <maximum symbol length>
--help (-h)

Example:
compressor.exe --compress -i in.txt -o out.smol --max_sym_len 256
Will compress in.txt with a max symbol length of 256, and output the result to out.smol