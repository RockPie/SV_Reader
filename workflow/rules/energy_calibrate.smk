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
             
rule energy_calibrate:
    conda:
        "general"
    log:
        "logs/energy_calibrate/energy_calibrate_{run_number}.log"
    resources:
        mem_mb = 10000,
        runtime_min = 60
    output:
        hitmaps = "dump/hitmaps_calib/hitmap_calib_run{run_number}.root",
        energy_compare = "dump/calib_compare/energy_compare_run{run_number}.png",
        hist_hg_csv = "dump/energy_hist/hist_hg_run{run_number}.csv",
        hist_calib_csv = "dump/energy_hist/hist_calib_run{run_number}.csv",
    input:
        data = "dump/parsed_root/parsed_run{run_number}.root",
        correlation = "dump/hglg_correlation/hl_correlation_run{run_number}.root",
        mapping = "data/config/Mapping_tb2023Sep_VMM3.csv"
    shell:
        "build/Energy_reconstruction -m {input.mapping} -d {input.data} -c {input.correlation} -h {output.hitmaps} -e {output.energy_compare} -g {output.hist_hg_csv} -n {output.hist_calib_csv} > {log}"