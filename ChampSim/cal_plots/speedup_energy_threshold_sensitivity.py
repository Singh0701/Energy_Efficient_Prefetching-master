import matplotlib.pyplot as plt
import numpy as np


#labels = ['SPP', 'PPF', 'IPCP L1D', 'IPCP', 'Bingo']

labels = ['A', 'B', 'C', 'D', 'E']

relaxation = [[1.146022094, 1.178829851, 1.165467332, 1.177029901, 1.179147919],[1.035954544, 1.028289133, 1.054773707, 1.059091878, 1.029606121], [0.9043276138, 0.8724289363, 0.8380654017, 0.8789376495, 0.8737180014]]
thresh50 = [[1.14205009, 1.167130566, 1.171115118, 1.175674749, 1.175391434],[1.037289119, 1.036217617, 1.055423005, 1.061109021, 1.027545902], [0.9077274714, 0.8820504257, 0.8782138606, 0.879160849, 0.8756212082]]
thresh40 = [[1.143866534, 1.174393254, 1.16790599, 1.17553299, 1.175851026],[1.034747981, 1.029678151, 1.05446435, 1.059020937, 1.027472556], [0.9047855331, 0.8749910369, 0.8788075172, 0.8780547417, 0.8752743995]]
thresh30 = [[1.143954947, 1.174723023, 1.168305384, 1.176074322, 1.176368071],[1.03525657, 1.030046697, 1.054738007, 1.059277814, 1.0274245], [0.9048216972, 0.8747446099, 0.8785039766, 0.8776336151, 0.8748886031]]
thresh20 = [[1.144551093, 1.174724105, 1.169725821, 1.177060143, 1.176392876],[1.035446034, 1.030305594, 1.056754052, 1.061445086, 1.02822871], [0.9048753388, 0.8747451388, 0.8777327521, 0.8769230194, 0.8748693844]]
thresh10 = [[1.1454983, 1.175704859, 1.169539537, 1.176917439, 1.177375411],[1.036528, 1.031125223, 1.058848787, 1.063073693, 1.02916955], [0.9049194832, 0.8741675191, 0.8774837127, 0.8772347484, 0.874141219]]


x = np.arange(len(labels))  # the label locations
width = 0.18  # the width of the bars

x1 = [0, 1.4, 2.8, 4.2, 5.6]
x2 = [0.2, 1.6, 3, 4.4, 5.8]
x3 = [0.4, 1.8, 3.2, 4.6, 6]
x4 = [0.6, 2, 3.4, 4.8, 6.2]
x5 = [0.8, 2.2, 3.6, 5, 6.4]
x6 = [1, 2.4, 3.8, 5.2, 6.6]

x_ticks = [0.5, 1.9, 3.3, 4.7, 6.1]

fig, ax = plt.subplots(ncols = 3, figsize = (12,8))
#green: #73C6B6
#yellow: #F7DC6F
for iteration in range(0,3):
    rects1 = ax[iteration].bar(x1, relaxation[iteration], width, label='Threshold Relaxation', edgecolor='k', align='center', color = '#5DADE2', linewidth=0.5)
    rects2 = ax[iteration].bar(x2, thresh50[iteration], width, label='1x Thresholds', edgecolor='k', align='center', color = '#F9E79F', linewidth=0.5)
    rects3 = ax[iteration].bar(x3, thresh40[iteration], width, label='0.8x Thresholds', edgecolor='k', align='center', color = '#34495E', linewidth=0.5)
    rects4 = ax[iteration].bar(x4, thresh30[iteration], width, label='0.6x Thresholds', edgecolor='k', align='center', color = '#A569BD', linewidth=0.5)
    rects5 = ax[iteration].bar(x5, thresh20[iteration], width, label='0.4x Thresholds', edgecolor='k', align='center', color = '#E59866', linewidth=0.5)
    rects6 = ax[iteration].bar(x6, thresh10[iteration], width, label='0.2x Thresholds', edgecolor='k', align='center', color = '#F2F3F4', linewidth=0.5)


    if(iteration == 0):
        ax[iteration].set_xticks(x_ticks)
        ax[iteration].set_xticklabels(labels, fontsize = 9.5)
        ax[iteration].set_ylim(1.14, 1.181)
        ax[iteration].yaxis.set_ticks(np.arange(1.14, 1.19, 0.02))
        ax[iteration].set_yticklabels(['1.14','1.16','1.18'], fontsize = 8)

        ax[iteration].set_xlabel('Speedup', fontsize = 9.5)
        
        ax[iteration].grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0)
        ax[iteration].set_axisbelow(True)
        ax[iteration].axhline(y=1, color='red', linestyle='--', linewidth = 0.5)
    if(iteration == 1):
        ax[iteration].set_xticks(x_ticks)
        ax[iteration].set_xticklabels(labels, fontsize = 9.5)
        ax[iteration].set_ylim(1.02, 1.07)
        ax[iteration].yaxis.set_ticks(np.arange(1.02, 1.07, 0.02))
        ax[iteration].set_yticklabels(['1.02','1.04','1.06'], fontsize = 8)
        ax[iteration].set_xlabel('Dynamic energy', fontsize = 9.5)
        ax[iteration].grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0)
        ax[iteration].set_axisbelow(True)
        ax[iteration].axhline(y=1, color='red', linestyle='--', linewidth = 0.5)
    if(iteration == 2):
        ax[iteration].set_xticks(x_ticks)
        ax[iteration].set_xticklabels(labels, fontsize = 9.5)
        ax[iteration].set_ylim(0.83, 0.91)
        ax[iteration].yaxis.set_ticks(np.arange(0.84, 0.91, 0.03))
        ax[iteration].set_yticklabels(['0.84','0.87','0.9'], fontsize = 8)
        ax[iteration].set_xlabel('Static energy', fontsize = 9.5)
        ax[iteration].grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0)
        ax[iteration].set_axisbelow(True)
        ax[iteration].axhline(y=1, color='red', linestyle='--', linewidth = 0.5)

 

# Add some text for labels, title and custom x-axis tick labels, etc.
ax[0].set_ylabel('Normalized to\nno prefetching', fontsize = 9.5)
#ax.set_title('Scores by group and gender')


ax[0].legend(handlelength=0.8, handletextpad = 0.5, labelspacing=0.2, ncol=3, framealpha=0, fancybox=True, loc = (0,0.9), fontsize=9.5)

ax[0].text(3, 1.202, "A: SPP, B: PPF, C: IPCP L1D, D: IPCP, E: Bingo", fontsize = 9.5)


fig.tight_layout()
plt.subplots_adjust(right = 0.43, top = 0.16, wspace = 0.27)

plt.show()
