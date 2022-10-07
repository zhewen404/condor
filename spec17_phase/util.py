bench_num_name_map = {
#     500: 'perlbench_r',
#     502: 'cpugcc_r',
    503: 'bwaves_r',
    505: 'mcf_r',
    507: 'cactuBSSN_r',
    525: 'x264_r',
    557: 'xz_r',
}

bench_num_unit_map = {
    503: '100',
    505: '1000',
    507: '100',
    525: '100',
    557: '100',
}

bench_num_command_map = {
#     500: '-I./lib checkspam.pl 2500 5 25 11 150 1 1 1 1',
#     502: 'gcc-pp.c -O3 -finline-limit=0 -fif-conversion -fif-conversion2 -o gcc-pp.opts-O3_-finline-limit_0_-fif-conversion_-fif-conversion2.s',
    503: 'bwaves_1 < bwaves_1.in',
    505: 'inp.in',
    507: 'spec_ref.par',
    525: '--pass 1 --stats x264_stats.log --bitrate 1000 --frames 1000 -o BuckBunny_New.264 BuckBunny.yuv 1280x720',
    557: 'cld.tar.xz 160 19cf30ae51eddcbefda78dd06014b4b96281456e078ca7c13e1c0c9e6aaea8dff3efb4ad6b0456697718cede6bd5454852652806a657bb56e07d61128434b474 59796407 61004416 6',
    
}

no_loop_end_list = [500,\
                    505,\
                    557,\
                   ]

bench_label_map = {
    503: 'static',
    505: 'config',    
    507: 'static',   
    525: 'static',
    557: 'static',    
}