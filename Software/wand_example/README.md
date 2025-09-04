重要提醒：
wand是魔杖最终烧录进去的代码，记得检查一下代码靠下方有一个数据打印的输出，需要检查一下和自己使用的轴，getmpudata录数据的轴是不是同样的轴！！！！
在getmpudata里面：
Serial.print(Ox);
Serial.print(",");
Serial.print(Oz);
那么这就是使用的x和z轴，在wand底下找到和这个相似的代码，修改成一样的
在wand_example里面：  
input[i * input_dim] = Ox;
input[i * input_dim + 1] = Oz;
这一段就是输入轴的数据了，请务必保证和上面是对应的。
