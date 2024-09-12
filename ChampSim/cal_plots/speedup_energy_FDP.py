import matplotlib.pyplot as plt
import numpy as np


#labels = ['SPP', 'PPF', 'IPCP L1D', 'IPCP', 'Bingo']

labels = ['A', 'B', 'C', 'D', 'E']


all_pref = [[1.157038254, 1.18586623, 1.168095945, 1.185689956, 1.198125969],[1.054027911, 1.063901782, 1.061386913, 1.073792654, 1.03904816],[0.9035560285, 0.8752115996, 0.8811979681, 0.8760561704, 0.8622118812]]
crit_pref = [[1.146022094, 1.178829851, 1.165467332, 1.177029901, 1.179147919],[1.035954544, 1.028289133, 1.054773707, 1.059091878, 1.029606121],[0.9043276138, 0.8724289363, 0.8380654017, 0.8789376495, 0.8737180014]]
all_pref_fdp = [[1.170767479, 1.186395098, 1.163237352, 1.174265828, 1.09436796],[1.06458859, 1.056588458, 1.049726017, 1.061136492, 1.027505991],[0.894616253, 0.8759433356, 0.884082876, 0.8831275265, 0.9304160503]]
crit_pref_fdp = [[1.156911847, 1.170157087, 1.159992013, 1.168285757, 1.082966695],[1.047134506, 1.039229577, 1.042419141, 1.04713497, 1.018708644],[0.897628274, 0.8834642959, 0.8842301594, 0.8820508921, 0.9384169622]]



x = np.arange(len(labels))  # the label locations
width = 0.18  # the width of the bars

x1 = [0, 1, 2, 3, 4]
x2 = [0.2, 1.2, 2.2, 3.2, 4.2]
x3 = [0.4, 1.4, 2.4, 3.4, 4.4]
x4 = [0.6, 1.6, 2.6, 3.6, 4.6]

x_ticks = [0.3, 1.3, 2.3, 3.3, 4.3]

fig, ax = plt.subplots(ncols = 3, figsize = (12,8))
#green: #73C6B6
#yellow: #F7DC6F
for iteration in range(0,3):
    rects1 = ax[iteration].bar(x1, all_pref[iteration], width, label='All IP Prefetch', edgecolor='k', align='center', color = '#E59866', linewidth=0.5)
    rects2 = ax[iteration].bar(x2, crit_pref[iteration], width, label='Critical IP Prefetch', edgecolor='k', align='center', color = '#5DADE2', linewidth=0.5)
    rects3 = ax[iteration].bar(x3, all_pref_fdp[iteration], width, label='All IP Prefetch FDP', edgecolor='k', align='center', color = '#F8BBD0', linewidth=0.5)
    rects4 = ax[iteration].bar(x4, crit_pref_fdp[iteration], width, label='Critical IP Prefetch FDP', edgecolor='k', align='center', color = '#A569BD', linewidth=0.5)
    #rects5 = ax[iteration].bar(x5, rob_occu[iteration], width, label='ROB Occupancy', edgecolor='k', align='center', color = '#5DADE2', linewidth=0.5)

    if(iteration == 0):
        ax[iteration].set_xticks(x_ticks)
        ax[iteration].set_xticklabels(labels, fontsize = 9.5)
        ax[iteration].set_ylim(1.07, 1.21)
        ax[iteration].set_yticklabels(['','1.1','1.2'], fontsize = 9)

        ax[iteration].set_xlabel('Speedup', fontsize = 9.5)
        
        ax[iteration].grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0)
        ax[iteration].set_axisbelow(True)
        ax[iteration].axhline(y=1, color='red', linestyle='--', linewidth = 0.5)
    if(iteration == 1):
        ax[iteration].set_xticks(x_ticks)
        ax[iteration].set_xticklabels(labels, fontsize = 9.5)
        ax[iteration].set_ylim(1, 1.08)
        ax[iteration].set_yticklabels(['1','1.05'], fontsize = 9)
        ax[iteration].set_xlabel('Dynamic energy', fontsize = 9.5)
        ax[iteration].grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0)
        ax[iteration].set_axisbelow(True)
        ax[iteration].axhline(y=1, color='red', linestyle='--', linewidth = 0.5)
    if(iteration == 2):
        ax[iteration].set_xticks(x_ticks)
        ax[iteration].set_xticklabels(labels, fontsize = 9.5)
        ax[iteration].set_ylim(0.82, 0.95)
        ax[iteration].set_yticklabels(['','0.85','0.9','0.95'], fontsize = 9)
        ax[iteration].set_xlabel('Static energy', fontsize = 9.5)
        ax[iteration].grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0)
        ax[iteration].set_axisbelow(True)
        ax[iteration].axhline(y=1, color='red', linestyle='--', linewidth = 0.5)



 

# Add some text for labels, title and custom x-axis tick labels, etc.
ax[0].set_ylabel('Normalized to\nno prefetching', fontsize = 9.5)
#ax.set_title('Scores by group and gender')


ax[0].legend(handlelength=0.8, handletextpad = 0.5, labelspacing=0.2, ncol=2, framealpha=0, fancybox=True, loc = (0.45,1.005), fontsize=9.5)

ax[0].text(3, 1.3, "A: SPP, B: PPF, C: IPCP L1D, D: IPCP, E: Bingo", fontsize = 9.5)


fig.tight_layout()
plt.subplots_adjust(right = 0.42, top = 0.16, wspace = 0.33)

plt.show()
