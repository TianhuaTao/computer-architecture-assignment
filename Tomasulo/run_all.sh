cd TestCase
rm -rf *.log
cd ..
rm -rf detailed_log log
mkdir detailed_log
mkdir Log

echo "Run TestCase/0.basic.nel"
./Tomasulo -v TestCase/0.basic.nel > detailed_log/0.basic.detail.log

echo "Run TestCase/1.basic.nel"
./Tomasulo -v TestCase/1.basic.nel > detailed_log/1.basic.detail.log

echo "Run TestCase/2.basic.nel"
./Tomasulo -v TestCase/2.basic.nel > detailed_log/2.basic.detail.log

echo "Run TestCase/3.basic.nel"
./Tomasulo -v TestCase/3.basic.nel > detailed_log/3.basic.detail.log

echo "Run TestCase/4.basic.nel"
./Tomasulo -v TestCase/4.basic.nel > detailed_log/4.basic.detail.log

echo "Run TestCase/Example.nel"
./Tomasulo -v TestCase/Example.nel > detailed_log/Example.detail.log

echo "Run TestCase/Big_test.nel"
./Tomasulo TestCase/Big_test.nel > detailed_log/Big_test.detail.log

echo "Run TestCase/Fabo.nel"
./Tomasulo -v TestCase/Fabo.nel > detailed_log/Fabo.detail.log

echo "Run TestCase/Fact.nel"
./Tomasulo -v TestCase/Fact.nel > detailed_log/Fact.detail.log

echo "Run TestCase/Mul.nel"
./Tomasulo TestCase/Mul.nel > detailed_log/Mul.detail.log

echo "Run TestCase/Gcd.nel"
./Tomasulo TestCase/Gcd.nel > detailed_log/Gcd.detail.log

echo "Run TestCase/sum.nel"
./Tomasulo -v TestCase/sum.nel > detailed_log/sum.detail.log

echo "Run TestCase/sum_no_jump.nel"
./Tomasulo -v TestCase/sum_no_jump.nel > detailed_log/sum_no_jump.detail.log

mv TestCase/*.log Log/
cd Log
for filename in *.log; do mv "$filename" "2017010255_$filename"; done;