import pandas as pd
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import sys
import argparse

# Try to set an interactive backend if not already set
# Common backends: 'TkAgg', 'Qt5Agg', 'WebAgg'
try:
    if matplotlib.get_backend() == 'agg':
        for backend in ['TkAgg', 'Qt5Agg', 'WebAgg', 'MacOSX']:
            try:
                matplotlib.use(backend)
                break
            except:
                continue
except:
    pass

def visualize(macro_file, micro_file, save_path=None):
    # Load data
    try:
        macro_df = pd.read_csv(macro_file)
        micro_df = pd.read_csv(micro_file)
    except Exception as e:
        print(f"Error reading CSV files: {e}")
        return

    # Map states to colors
    color_map = {0: 'blue', 1: 'red', 2: 'green', 3: 'orange'}
    
    times = micro_df['Time'].unique()

    fig, (ax_spatial, ax_macro) = plt.subplots(1, 2, figsize=(15, 6))

    ax_spatial.set_aspect('equal')
    ax_spatial.set_title('Spatial Agent View')
    scatter = ax_spatial.scatter([], [], c=[], s=20)
    
    ax_macro.set_title('SIR Epidemic Curves')
    ax_macro.set_xlabel('Time')
    ax_macro.set_ylabel('Count')
    line_s, = ax_macro.plot([], [], label='Susceptible', color='blue')
    line_i, = ax_macro.plot([], [], label='Infected', color='red')
    line_r, = ax_macro.plot([], [], label='Recovered', color='green')
    ax_macro.legend()

    ax_spatial.set_xlim(micro_df['X'].min() - 5, micro_df['X'].max() + 5)
    ax_spatial.set_ylim(micro_df['Y'].min() - 5, micro_df['Y'].max() + 5)
    
    ax_macro.set_xlim(macro_df['Time'].min(), macro_df['Time'].max())
    ax_macro.set_ylim(0, macro_df[['Susceptible', 'Infected', 'Recovered']].values.max() * 1.1)

    def update(frame_time):
        frame_data = micro_df[micro_df['Time'] == frame_time]
        scatter.set_offsets(frame_data[['X', 'Y']])
        scatter.set_color([color_map.get(s, 'black') for s in frame_data['State']])

        history = macro_df[macro_df['Time'] <= frame_time]
        line_s.set_data(history['Time'], history['Susceptible'])
        line_i.set_data(history['Time'], history['Infected'])
        line_r.set_data(history['Time'], history['Recovered'])

        return scatter, line_s, line_i, line_r

    ani = animation.FuncAnimation(fig, update, frames=times, interval=50, blit=True)
    plt.tight_layout()

    if save_path:
        print(f"Saving animation to {save_path}...")
        if save_path.endswith('.gif'):
            ani.save(save_path, writer='pillow')
        else:
            ani.save(save_path, writer='ffmpeg')
        print("Done.")
    else:
        try:
            plt.show()
        except Exception as e:
            print(f"Could not show plot: {e}")
            print("Try running with --save output.gif")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Visualize SIR Simulation')
    parser.add_argument('macro', nargs='?', default='macro.csv', help='Macro CSV file')
    parser.add_argument('micro', nargs='?', default='micro.csv', help='Micro CSV file')
    parser.add_argument('--save', help='Save animation to file (e.g. output.gif or output.mp4)')
    
    args = parser.parse_args()
    visualize(args.macro, args.micro, args.save)
