fn1 = 0
fn0 = 1
fn = 0

for x in range (1,30):
    fn = fn0 + fn1
    fn1 = fn0
    fn0 = fn
    print('%d %d %d\n' % (fn, fn1, fn0))