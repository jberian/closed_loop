import csv

def calc_stats(dir):
  with open(dir+'/performance_stats.csv') as csv_file:
    num_adults = 0
    adults_tir = 0.0
    adults_low = 0.0
    adults_high = 0.0
    adults_veryhigh = 0.0
    num_children = 0
    children_tir = 0.0
    children_low = 0.0
    children_high = 0.0
    children_veryhigh = 0.0
    num_adolescents = 0
    adolescents_tir = 0.0
    adolescents_low = 0.0
    adolescents_high = 0.0
    adolescents_veryhigh = 0.0
    csv_reader = csv.reader(csv_file, delimiter=',')
    for row in csv_reader:
        if 'child' in row[0]:
            num_children += 1
            children_tir += float(row[1])
            children_high += float(row[2])
            children_low += float(row[3])
            children_veryhigh += float(row[4])
        if 'adult' in row[0]:
            num_adults += 1
            adults_tir += float(row[1])
            adults_high += float(row[2])
            adults_low += float(row[3])
            adults_veryhigh += float(row[4])
        if 'adolescent' in row[0]:
            num_adolescents += 1
            adolescents_tir += float(row[1])
            adolescents_high += float(row[2])
            adolescents_low += float(row[3])
            adolescents_veryhigh += float(row[4])
    children_tir /= num_children
    children_high /= num_children
    children_low /= num_children
    children_veryhigh /= num_children
    adolescents_tir /= num_adolescents
    adolescents_high /= num_adolescents
    adolescents_low /= num_adolescents
    adolescents_veryhigh /= num_adolescents
    adults_tir /= num_adults
    adults_low /= num_adults
    adults_high /= num_adults
    adults_veryhigh /= num_adults

    print('Children: (n='+str(num_children)+')', \
          'Low: ' + str(children_low), \
          'In Range: ' + str(children_tir), \
          'High: ' + str(children_high), \
          'Very High: ' + str(children_veryhigh))
    print('Adolescents: (n='+str(num_adolescents)+')', \
          'Low: ' + str(adolescents_low), \
          'In Range: ' + str(adolescents_tir), \
          'High: ' + str(adolescents_high), \
          'Very High: ' + str(adolescents_veryhigh))
    print('Adults: (n='+str(num_adults)+')', \
          'Low: ' + str(adults_low), \
          'In Range: ' + str(adults_tir), \
          'High: ' + str(adults_high), \
          'Very High: ' + str(adults_veryhigh))

print('Batch 1')
calc_stats('results_batch1')
print(' ')
print('Batch 2')
calc_stats('results_batch2')
print(' ')
print('Batch 3')
calc_stats('results_batch3')
print(' ')
print('Batch 4')
calc_stats('results_batch4')
print(' ')
