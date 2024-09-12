import matplotlib.pyplot as plt
import numpy as np


#labels = ['SPP', 'PPF', 'IPCP L1D', 'IPCP', 'Bingo']

labels = ['A', 'B', 'C', 'D', 'E']

rob_occu = [[1.146022094, 1.178829851, 1.165467332, 1.177029901, 1.179147919], [1.035954544, 1.028289133, 1.054773707, 1.059091878, 1.029606121], [0.9043276138, 0.8724289363, 0.8380654017, 0.8789376495, 0.8737180014]]
fvp_cd = [[1.13451249, 1.165707078, 1.170798694, 1.177714576, 1.169271612], [1.034103281, 1.029818111, 1.05991559, 1.065418536, 1.028321259], [0.9115901984, 0.8809398972, 0.8779461876, 0.8759370149, 0.8805456651]]
hpca_2009 = [[1.000320275, 1.000099866 , 0.9995655511, 0.9999073946, 1.000109594], [0.999879188, 0.9999149112, 1.000526214, 1.000593394, 1.000223865], [0.9997280957, 1.000175184, 1.000885157, 1.000523959, 1.000301194]]
focused_prefetching = [[1.129301925, 1.151858298, 1.15196846, 1.1614854, 1.156543366], [1.033362763, 1.028654058, 1.053571284, 1.057653496, 1.026808292], [0.9154183304, 0.8911593182, 0.8903785702, 0.8858242197, 0.8886628709]]
catch = [[1.09908765, 1.119097589, 1.144892138, 1.151875621, 1.12865228], [1.033111552, 1.030103164, 1.051206123, 1.056609547, 1.023625136], [0.9382367962, 0.9173831119, 0.8969284518, 0.8971332502, 0.9079484085]]

x = np.arange(len(labels))  # the label locations
width = 0.18  # the width of the bars

x1 = [0, 1.2, 2.4, 3.6, 4.8]
x2 = [0.2, 1.4, 2.6, 3.8, 5]
x3 = [0.4, 1.6, 2.8, 4, 5.2]
x4 = [0.6, 1.8, 3, 4.2, 5.4]
x5 = [0.8, 2, 3.2, 4.4, 5.6]

x_ticks = [0.4, 1.6, 2.8, 4, 5.2]

fig, ax = plt.subplots(ncols = 3, figsize = (12,8))
#green: #73C6B6
#yellow: #F7DC6F
for iteration in range(0,3):
    rects1 = ax[iteration].bar(x1, hpca_2009[iteration], width, label='Subramaniam et al.', edgecolor='k', align='center', color = '#E59866', linewidth=0.5)
    rects2 = ax[iteration].bar(x2, focused_prefetching[iteration], width, label='Focused Prefetching', edgecolor='k', align='center', color = '#34495E', linewidth=0.5)
    rects3 = ax[iteration].bar(x3, catch[iteration], width, label='CATCH', edgecolor='k', align='center', color = '#F8BBD0', linewidth=0.5)
    rects4 = ax[iteration].bar(x4, fvp_cd[iteration], width, label='FVP', edgecolor='k', align='center', color = '#A569BD', linewidth=0.5)
    rects5 = ax[iteration].bar(x5, rob_occu[iteration], width, label='ROB Occupancy', edgecolor='k', align='center', color = '#5DADE2', linewidth=0.5)

    if(iteration == 0):
        ax[iteration].set_xticks(x_ticks)
        ax[iteration].set_xticklabels(labels, fontsize = 9.5)
        ax[iteration].set_ylim(0.99, 1.2)
        ax[iteration].set_yticklabels(['','1','1.1','1.2'], fontsize = 9)

        ax[iteration].set_xlabel('Speedup', fontsize = 9.5)
        
        ax[iteration].grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0)
        ax[iteration].set_axisbelow(True)
        ax[iteration].axhline(y=1, color='red', linestyle='--', linewidth = 0.5)
    if(iteration == 1):
        ax[iteration].set_xticks(x_ticks)
        ax[iteration].set_xticklabels(labels, fontsize = 9.5)
        ax[iteration].set_ylim(1, 1.07)
        ax[iteration].set_yticklabels(['1','1.05'], fontsize = 9)
        ax[iteration].set_xlabel('Dynamic energy', fontsize = 9.5)
        ax[iteration].grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0)
        ax[iteration].set_axisbelow(True)
        ax[iteration].axhline(y=1, color='red', linestyle='--', linewidth = 0.5)
    if(iteration == 2):
        ax[iteration].set_xticks(x_ticks)
        ax[iteration].set_xticklabels(labels, fontsize = 9.5)
        ax[iteration].set_ylim(0.83, 0.95)
        ax[iteration].set_yticklabels(['','0.85','0.9','0.95'], fontsize = 9)
        ax[iteration].set_xlabel('Static energy', fontsize = 9.5)
        ax[iteration].grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0)
        ax[iteration].set_axisbelow(True)
        ax[iteration].axhline(y=1, color='red', linestyle='--', linewidth = 0.5)
        ax[iteration].text(-0.4, 0.951, '0.99', fontsize = 8.5)
        ax[iteration].text(1.1, 0.951, '1', fontsize = 8.5)
        ax[iteration].text(2.3, 0.951, '1', fontsize = 8.5)
        ax[iteration].text(3.5, 0.951, '1', fontsize = 8.5)
        ax[iteration].text(4.7, 0.951, '1', fontsize = 8.5)



 

# Add some text for labels, title and custom x-axis tick labels, etc.
ax[0].set_ylabel('Normalized to\nno prefetching', fontsize = 9.5)
#ax.set_title('Scores by group and gender')


ax[0].legend(handlelength=0.8, handletextpad = 0.5, labelspacing=0.2, ncol=3, framealpha=0, fancybox=True, loc = (0.2,1), fontsize=9.5)

ax[0].text(3.5, 1.33, "A: SPP, B: PPF, C: IPCP L1D, D: IPCP, E: Bingo", fontsize = 9.5)


fig.tight_layout()
plt.subplots_adjust(right = 0.42, top = 0.16, wspace = 0.35)

plt.show()
