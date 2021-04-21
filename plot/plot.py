from pathlib import Path
import json
import numpy as np
import pandas as pd
import seaborn as sns


def read_benchmark_json(path):
    with open(path) as file:
        content = json.load(file)
    df = pd.DataFrame(content['benchmarks'])
    df['run_name'] = df['run_name'].str[:-len('/manual_time')]
    df[['scenario', 'lock']] = df['run_name'].str.split(
        pat='/', expand=True)[[0, 1]]

    df = df.join(pd.json_normalize(df['label'].map(json.loads)))
    return df


plot_dir = Path(__file__).resolve().parent
reports_dir = plot_dir / '../reports'
for commit_path in reports_dir.iterdir():
    commit = commit_path.name
    json_dir = commit_path / 'json'
    png_dir = commit_path / 'png'
    if png_dir.is_dir():
        print('Skipping '+commit)
        continue
    print('Plotting '+commit)

    data = pd.concat([
        read_benchmark_json(p)
        for p in json_dir.iterdir()
        if p.suffix == '.json'
    ])

    png_dir.mkdir(exist_ok=True)

    raw = data[data['run_type'] == 'iteration']
    for (scenario, df) in raw.groupby('scenario'):
        df['critical_sections'] = df['critical_sections']/1000000

        plot = sns.lineplot(
            data=df,
            estimator=np.median,
            x='mpi_processes', y='critical_sections',
            hue='lock', style='lock', markers=True, dashes=False,
        )
        plot.set(
            ylim=0,
            # yscale='log',
            ylabel='median throughput in million locks/s\nwith 95% confidence interval',
            xticks=df['mpi_processes'].drop_duplicates(),
        )

        # if 'acquire_immediate_count' in df.columns:
        #     ax2 = plot.twinx()
        #     df['acquire_immediate_proportion'] = \
        #         df['acquire_immediate_count'] / df['acquire_count']
        #     sns.lineplot(
        #         ax=ax2,
        #         data=df,
        #         x='mpi_processes', y='acquire_immediate_proportion',
        #         color='b',
        #     )
        #     ax2.set(ylim=(0, 1))

        # if 'spin_count' in df.columns:
        #     ax2 = plot.twinx()
        #     df['spin_proportion'] = df['spin_count'] / df['acquire_count']
        #     sns.lineplot(
        #         ax=ax2,
        #         data=df,
        #         x='mpi_processes', y='spin_proportion',
        #         color='r',
        #     )
        #     ax2.set(ylim=0)

        fig = plot.get_figure()
        fig.savefig(png_dir / (scenario+'.png'))
        fig.clf()
