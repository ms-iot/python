print("Hello World")

fn1 = 0
fn0 = 1
fn = 0

print(fn1)
print(fn0)

for x in range (1,30):
    fn = fn0 + fn1
    print(fn)
    fn1 = fn0
    fn0 = fn