import numpy as np
import matplotlib.pyplot as plt
from matplotlib import rc

rc('font')


# => SPP, PPF, IPCP L1, IPCP L1+L2, Bingo L2

# Speedup, Dynamic energy, Static energy

all_ip_pref = [[1.157038254, 1.18586623, 1.168095945, 1.185689956, 1.198125969], [1.054027911, 1.063901782, 1.061386913, 1.073792654, 1.03904816], [0.9035560285, 0.8752115996, 0.8811979681, 0.8760561704, 0.8622118812]]

crit_ip_pref = [[1.146022094, 1.178829851, 1.165467332, 1.177029901, 1.179147919], [1.036173417, 1.028508468, 1.054993566, 1.059312641, 1.029825125], [0.9043276138, 0.8724289363, 0.8380654017, 0.8789376495, 0.8737180014]]


labels = ["Speedup", "Dynamic energy", "Static energy"]

#fig = plt.figure(figsize = (10, 5))
fig, ax = plt.subplots(ncols = 3, figsize = (12,8))
# creating the bar plot

#r = [0.2,0.5,0.8,1.1,1.4,1.7]
r = [0.2,0.5,0.8,1.1,1.4]

colors = ['#C39BD3', '#82E0AA', '#85C1E9']

for iteration in range(0,3):

#b1 = ax.bar(r, energy_static, color ='#ABFFC2', width = 0.2, align='center', edgecolor='black')
    b2 = ax[iteration].bar(r, all_ip_pref[iteration], color = colors[iteration] , width = 0.2, align='center', edgecolor='black')
    b3 = ax[iteration].bar(r, crit_ip_pref[iteration], color = 'none', hatch = '///', width = 0.2, align='center', edgecolor='black')
#b4 = ax[iteration].bar(r, critical_energy_static, color = 'none', bottom = np.array(critical_energy_static), hatch = '...', width = 0.2, align='center', edgecolor='black')

    #ax[iteration].xax[iteration]is.set_ticks(np.arange(0.2, 1.9, 0.3))

    ax[iteration].xaxis.set_ticks(np.arange(0.2, 1.6, 0.3))
    ax[iteration].set_xlabel(labels[iteration], fontsize=9.5)

    #ax[iteration].set_xticklabels(("SPP", "PPF", "IPCP\nL1D", "IPCP", "Bingo"), rotation=90, fontsize=9.5, linespacing=0.7)
    ax[iteration].set_xticklabels(("A", "B", "C", "D", "E"), fontsize=9.5, linespacing=0.7)
    if(iteration == 0):
        #ax[iteration].set_xticklabels(("S2", "I1", "I12", "B2", "P2"))
        #ax[iteration].set_xticklabels(("SPP", "IPCP\nL1", "IPCP", "Bingo", "PPF"), rotation=90, fontsize=9.5, linespacing=0.7)
        ax[iteration].set_ylabel("Normalized to\nno prefetching", fontsize=10)
        ax[iteration].set_ylim(1.14, 1.205)
        ax[iteration].set_yticklabels(['','1.15','1.2'], fontsize=9.5)
        #ax[iteration].set_yticklabels(['-25','-20','-15','-10'], fontsize=9.5)
        #ax[iteration].set_yticklabels(['0', '0.2', '0.4', '0.6','0.8', '1', '1.2'], fontsize=8)
    elif(iteration == 1):
        #ax[iteration].set_xticklabels(("S2", "P2", "I1", "I12", "B2"))
        #ax[iteration].set_xticklabels(("SPP", "PPF", "IPCP\nL1", "IPCP", "Bingo"), rotation=90, fontsize=9.5, linespacing=0.7)
        ax[iteration].set_ylim(1.02, 1.08)
        ax[iteration].yaxis.set_ticks(np.arange(1.02, 1.082, 0.02))
        ax[iteration].set_yticklabels(['1.02','1.04','1.06','1.08'], fontsize=9.5)
        #ax[iteration].set_yticklabels(['-11','-7', '-3'], fontsize=9.5)
    elif(iteration == 2):
        ax[iteration].set_ylim(0.82, 0.92)
        #ax[iteration].set_xticklabels(("I1", "S2", "I12", "P2", "B2"))
        #ax[iteration].set_xticklabels(("IPCP\nL1", "SPP", "IPCP", "PPF", "Bingo"), rotation=90, fontsize=9.5, linespacing=0.7)
        #ax[iteration].yaxis.set_ticks(np.arange(0.90, 0.98, 0.03))
        ax[iteration].axhline(y=1, color='red', linestyle='-', linewidth = 0.5)
        ax[iteration].set_yticklabels(['','0.85','0.9'], fontsize=9.5)
        #ax[iteration].set_yticklabels(['-10','-7','-4'], fontsize=9.5)

    #ax[iteration].xlabel("Courses offered")
    #ax[iteration].set_ylabel("Normalized Dynamic Energy\nw.r.t. No Prefetching", fontsize=8)
    #plt.title("Students enrolled in different courses")
    #ax[iteration].set_yticklabels(['0', '0.2', '0.4', '0.6','0.8', '1', '1.2'], fontsize=8)
    ax[iteration].grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0) 
    ax[iteration].set_axisbelow(True)


ax[0].legend((b2[0], b3[0]), ("All IP Prefetch", "Critical IP Prefetch"), handlelength=0.8, handletextpad = 0.5, labelspacing=0.2, ncol=2, framealpha=0, fancybox=True, loc = (0.5, 1), fontsize=9.5) 

ax[0].text(0.7, 1.232, "A: SPP, B: PPF, C: IPCP L1D, D: IPCP, E: Bingo", fontsize = 9.5)

leg = ax[0].get_legend()
leg.legendHandles[0].set_color('k')
#leg.legendHandles[1].set_color('yellow')

#ax.grid(color='lightgrey', which='major', axis='y', linestyle='solid', zorder = 0) 
#ax.set_axisbelow(True)
#ax.axhline(y=1, color='red', linestyle='-', linewidth = 0.5)

plt.subplots_adjust(wspace=0.45, hspace=0, right = 0.42, top = 0.2)
#plt.savefig('SavingPlots/pdfs/FinFet_allTotalStaticEnergy_subplots_percent.pdf', bbox_inches = 'tight', pad_inches = 0.1)
#plt.savefig('SavingPlots/pngs/FinFet_allTotalStaticEnergy_subplots_percent.png', bbox_inches = 'tight', pad_inches = 0.1)
plt.show()
