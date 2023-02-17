import random

i = 0
number = []
print("number: ")
while i < 15:
  number.append(random.randint(0,255))
  i += 1
print(*number)
for n in number:
  print('%#x'%n,end=' ')