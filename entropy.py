import math 

Total = 15

def getEntropy(cnt, total):
    el = (total - cnt)
    v1 = cnt/total
    log1 = math.log2(v1)
    v2 = el/total
    log2 = math.log2(v2)
    entropy = -1*(v1*log1) + -1*(v2*log2)   
    return entropy 

for i in range(Total):
    if i > 0:
        entropy = getEntropy(i, Total)
        if i > (Total / 2):        
            entropy = 1 + (1 - entropy)
        print(i, 1/entropy)    
    


