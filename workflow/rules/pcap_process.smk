configfile: 'workflow/configs/config.yaml'

rule pcap_process:
    conda:
        "general"
    log:
        "logs/pcap_process/pcap_process_{run_number}.log"
    output:
        raw = "dump/raw_root/raw_run{run_number}.root",
        parsed = "dump/parsed_root/parsed_run{run_number}.root",
        browse = "dump/browse_root/browse_run{run_number}.root"
    input:
        data = "data/Run{run_number}",
        mapping = "data/config/Mapping_tb2023Sep_VMM3.csv"
    shell:
        "build/data_inspection -m {input.mapping} -d {input.data} -r {output.raw} -p {output.parsed} -a {output.browse} > {log}"
