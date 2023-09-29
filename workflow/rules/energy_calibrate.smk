configfile: 'workflow/configs/config.yaml'

import os

rule hglg_correlation:
    params:
        save_png = 'false'
    log:
        "logs/hglg_correlation/hl_correlation_{run_number}.log"
    output:
        "dump/hglg_correlation/hl_correlation_run{run_number}.root"
    input:
       data = "dump/parsed_root/parsed_run{run_number}.root",
       mapping = "data/config/Mapping_tb2023Sep_VMM3.csv"
    run:
        if params.save_png == 'true':
            if not os.path.exists("dump/hglg_pics"):
                os.makedirs("dump/hglg_pics")
        shell("build/HG_LG_Correlation -m {input.mapping} -i {input.data} -o {output} -p dump/hglg_pics -e {params.save_png} > {log}")
             