import tensorflow as tf
import pandas as pd
import math

# 设置数据集的形状
timesteps = 128
lr = 1e-4
num_epochs = 200
batch_size = 4
input_dim = 2#///////////////////////////////////////////////////////////////传感器收集数据的维度，比如你只录入了x轴和z轴，那就写2，类推。
num_classes = 5#/////////////////////////////////////////////////////////////类比的数量，比如说你录了4个动作，那就写5，类推

class Model(tf.keras.Model):
    def __init__(self):
        super(Model, self).__init__()
        self.conv1 = tf.keras.layers.Conv2D(filters=8, kernel_size=(1,3), strides=(1,1), padding='same', activation='relu')
        self.conv2 = tf.keras.layers.Conv2D(filters=16, kernel_size=(1,3), strides=(1,1), padding='same', activation='relu')
        self.flatten = tf.keras.layers.Flatten()
        self.fc1 = tf.keras.layers.Dense(16, activation='relu')
        self.fc2 = tf.keras.layers.Dense(num_classes, activation='softmax')

    def call(self, x):
        x = tf.transpose(x, perm=[0, 2, 1])
        x = tf.expand_dims(x, axis=2)
        x = self.conv1(x)
        x = self.conv2(x)
        x = self.flatten(x)
        x = self.fc1(x)
        x = self.fc2(x)

        return x

if __name__ == '__main__':

    train_x_pd = pd.read_csv('data_x.csv', header=None)#////////////////////////////改成你录入数据的文件
    train_y_pd = pd.read_csv('data_y.csv', header=None)#////////////////////////////改成你录入数据对应分类的文件
    test_x_pd = pd.read_csv('test_x.csv', header=None)#////////////////////////////这是一个用于测试的文件，和上面的两个文件一样的录入方法
    test_y_pd = pd.read_csv('test_y.csv', header=None)#////////////////////////////如果不想重新录就把名字改成和上面文件名的一样，比如test_x就改成data_x，类推

    train_x = tf.convert_to_tensor(train_x_pd.to_numpy(), dtype=tf.float32)
    train_x = tf.reshape(train_x, [-1, timesteps, input_dim])
    train_y = tf.convert_to_tensor(train_y_pd.to_numpy(), dtype=tf.int32)

    test_x = tf.convert_to_tensor(test_x_pd.to_numpy(), dtype=tf.float32)
    test_x = tf.reshape(test_x, [-1, timesteps, input_dim])
    test_y = tf.convert_to_tensor(test_y_pd.to_numpy(), dtype=tf.int32)

    train_data = tf.data.Dataset.from_tensor_slices((train_x, train_y))
    train_data = train_data.batch(batch_size)

    test_data = tf.data.Dataset.from_tensor_slices((test_x, test_y))
    test_data = test_data.batch(batch_size)

    train_size = tf.data.experimental.cardinality(train_data).numpy()
    test_size = tf.data.experimental.cardinality(test_data).numpy()
    steps_per_epoch = math.ceil(train_size / batch_size)
    validation_steps = math.ceil(test_size / batch_size)

    train_data = train_data.repeat()
    test_data = test_data.repeat()

    # 构建模型
    model = Model()

    # 编译模型
    model.compile(optimizer=tf.keras.optimizers.Adam(learning_rate=lr),
                loss=tf.keras.losses.SparseCategoricalCrossentropy(from_logits=False),
                metrics=['accuracy'])

    # 训练模型
    model.fit(train_data, epochs=num_epochs, steps_per_epoch=steps_per_epoch, validation_data=test_data, validation_steps=validation_steps)

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    
    tflite_model = converter.convert()

    with open('model.tflite', 'wb') as f:
        f.write(tflite_model)