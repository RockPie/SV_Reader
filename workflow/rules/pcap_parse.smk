configfile: 'workflow/configs/config.yaml'

data_path = config['path_data_directory']
exec_path = config['path_exec_directory']

print (data_path)

rule pcap_parse:
    input:
        data_path + '/Run{prefix}'
    output:
        'workflow/dump/analysis_rcslrm_Run{prefix}v.root'
    shell:
        'cd {exec_path} && ./data_inspection -r {wildcards.prefix} -i {input} -o {output}'