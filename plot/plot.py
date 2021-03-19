from pathlib import Path
import json
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
    png_dir.mkdir(exist_ok=True)

    data = pd.concat([read_benchmark_json(p) for p in json_dir.iterdir()])

    raw = data[data['run_type'] == 'iteration']
    for (scenario, df) in raw.groupby('scenario'):
        df['critical_sections'] = df['critical_sections']/1000000

        plot = sns.lineplot(
            data=df,
            x='mpi_processes', y='critical_sections',
            hue='lock', style='lock', markers=True, dashes=False
        )
        plot.set(
            ylabel='median throughput in million locks/s\nwith 95% confidence interval',
            xticks=df['mpi_processes'].drop_duplicates()
        )

        fig = plot.get_figure()
        fig.savefig(png_dir / (scenario+'.png'))
        fig.clf()
