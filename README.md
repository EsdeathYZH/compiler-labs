﻿# compiler-labs
### 说明
lab6为最终版本的编译器，修复了之前lab4，lab5的一些bug，所以最好直接参考lab6,在最终完成lab之后，我写了一个非常naive的shell脚本evaluate.sh，来统计testcase生成的汇编代码行数与各个函数分配栈大小的总和。因为我的代码中没有push，pop操作，所以只需要检查addq $constant, %rsp这样的指令就可以。
### 一个小优化
书上介绍了合并溢出的方式，但是不写合并溢出也能过完测试，我之后简单的实现了这一特性（没有合并阶段，实在懒得写了orz），只是在实际溢出后又有一个着色的过程，着色的过程也没有使用启发式的简化压栈，而是使用贪心这种随缘的方式。经过evaluate测试，栈帧分配历史的总和降低了12%，这也算是一个肉眼可见的优化吧。
### 64位无帧指针解决方案
+ 我在这里使用的是确定栈大小之后再修改指令的方式，在之前指令选择的时候，为每个需要用到framesize的地方打一个？#标记，之后rewrite扫描一遍就好。
+ 另外一种方式是在函数开头设置一个变量，之后怎么使用这个变量我没去具体实现，应该是要把这个变量move到一个临时变量里然后使用两寄存器寻址？
但是这样每次都还是要用一个寄存器存放framesize，那和使用rbp有啥区别呢？最后我没有想出更合适的解决方案就没有使用这个方法。

### 关于内存泄漏与运行效率
在写代码的时候因为无穷无尽的内存泄漏而感到窒息，但是为了不引入复杂性搞自己就没有写free，其实可以在某些肯定安全的地方
加一些free，让它漏的少一些，关于类似list的这种数据结构，我认为使用按照指针排序的有序列表会更加高效，我也在temp.c中写了一个
使用ordered list的集合运算查找增删的基础操作，但是为了不大幅改动寄存器分配抄的伪代码，就没有实现，写完发现编译速度挺快的，不需要优化。

### Bugs Report
1.字符串传地址的时候要用leaq label(%rip)来传了

2.在mul或者div的时候会改rdx，这有可能影响参数寄存器，所以我每次多了两条save and restore的指令（此处我的做法不能解决根本问题，正确做法是12，这个情况也是说明了为什么要进行12操作的例子）

3.div之前要加一句ctqo。。我也不知道为啥不加会报浮点异常，网上解释没看懂。

4.cmp操作数顺序别搞混了

5.因为参数寄存器可能会被函数中的call复用，所以我在call的时候把argregs放进dst里面了

6.我自己写的辣鸡编译器可能会生成movq %rsp...这种指令，所以我在每个MOVE指令之前都判断了一下，以便把rsp加上framesize

7.字符串段需要有一个.long 指令后面跟字符串长度，不然会报错，助教的代码没给这个。

8.生成控制流的时候不要忘了给jmp和它下一条指令也加边。。(这句话说错了，真正的bug是我没给cjump的AS_Target加false label)

9.寄存器分配在生成okcollor的时候我使用了浅拷贝，此处应该深拷贝过来一个registers

10.我使用PPT上的for伪代码，在线性化的时候不知道为什么会失去正确性，而我使用不考虑溢出的for伪代码却能过？？？

11.malloc后面要加@PLT，这也是64位的锅（刚发现allocRecord。。。是我傻了）

12.newFrame的时候，前六个寄存器代表的参数就算不逃逸也要生成转移的指令，以防之后的访问冲突。

13.没写字符串比较。。。。。。。。。。。。。。。。。。。我以为testcase里面没有的orz

14.runtime.c里面的函数的level和tigermain不能在同一个level的，之前为了debug尝试放在一个level忘记改回去了orz

15.array元素大小不一定是wordsize，intarray大小是4，这个bug找了好久orz，所以说如果array元素不是int的话不要调用initarray。。。中文书上有点误导人，但貌似测试用例里面全是整型数组就懒得写了。

16.translate模块在生成IR树的时候，要注意生成的顺序，要按照从左到右的顺序来生成这个树(尤其是Seq)，不然有可能会影响正确性，如果左边的句子有副作用的话。
