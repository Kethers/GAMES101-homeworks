# GAMES101-作业8

## 任务1：连接绳子的约束

又2个包含指针的vector，分别存储质点对象Mass和弹簧对象Spring，运用构造函数创建好质点和弹簧存入对应的vector中。

之后，根据注释和作业的说明，将对应索引的质点给钉死（固定住，即pinned设置为true）。

代码如下：

```cpp
Rope::Rope(Vector2D start, Vector2D end, int num_nodes, float node_mass, float k, vector<int> pinned_nodes)
{
    // TODO: (Part 1): Create a rope starting at `start`, ending at `end`, and containing `num_nodes` nodes.

    for (int i = 0; i < num_nodes; i++)
    {
        masses.push_back(new Mass(start + (double)i / (num_nodes - 1) * (end - start), node_mass, false));
    }
    for (int i = 0; i < num_nodes - 1; i++)
    {
        springs.push_back(new Spring(masses[i], masses[i + 1], k));
    }

    //        Comment-in this part when you implement the constructor
    for (auto &i : pinned_nodes)
    {
        masses[i]->pinned = true;
    }
}
```



## 任务2：显式/半隐式欧拉法

不知道为什么显式的方法会使得蓝色的节点以极快的速度飞出屏幕……

```cpp
void Rope::simulateEuler(float delta_t, Vector2D gravity)
{
    for (auto &s : springs)
    {
        // TODO: (Part 2): Use Hooke's law to calculate the force on a node
        Vector2D vec_m1_to_m2 = s->m2->position - s->m1->position;
        double length_m1_to_m2 = vec_m1_to_m2.norm();
        Vector2D f = s->k * vec_m1_to_m2 / length_m1_to_m2 * (length_m1_to_m2 - s->rest_length);
        s->m1->forces += f;
        s->m2->forces -= f;
    }

    for (auto &m : masses)
    {
        if (!m->pinned)
        {
            // TODO: (Part 2): Add the force due to gravity, then compute the new velocity and position
            m->forces += gravity * m->mass;

            // TODO: (Part 2): Add global damping
            float kd = 0.01;
            m->forces -= kd * m->velocity;
            Vector2D a = m->forces / m->mass;

            // explicit Euler
            // m->position += m->velocity * delta_t;
            // m->velocity += a * delta_t;

            // semi-implicit Euler
            m->velocity += a * delta_t;
            m->position += m->velocity * delta_t;
        }

        // Reset all forces on each mass
        m->forces = Vector2D(0, 0);
    }
}
```



## 任务3&4：显式Verlet及阻尼

```cpp
void Rope::simulateVerlet(float delta_t, Vector2D gravity)
{
    for (auto &s : springs)
    {
        // TODO: (Part 3): Simulate one timestep of the rope using explicit Verlet （solving constraints)
        Vector2D vec_m1_to_m2 = s->m2->position - s->m1->position;
        double length_m1_to_m2 = vec_m1_to_m2.norm();
        Vector2D f = s->k * vec_m1_to_m2 / length_m1_to_m2 * (length_m1_to_m2 - s->rest_length);
        s->m1->forces += f;
        s->m2->forces -= f;
    }

    for (auto &m : masses)
    {
        if (!m->pinned)
        {
            m->forces += gravity * m->mass;
            Vector2D a = m->forces / m->mass;

            // TODO: (Part 3.1): Set the new position of the rope mass
            Vector2D temp = m->position;
            // TODO: (Part 4): Add global Verlet damping
            double damping_factor = 0.00005;
            // 核心公式，带阻尼的计算
            m->position = m->position + (1 - damping_factor) * (m->position - m->last_position) + a * delta_t * delta_t;
            // 用last_position记忆原先的位置
            m->last_position = temp;
        }
    }
}
```



## 参考资料：

[GAMES系列课程作业与笔记 - 知乎 (zhihu.com)](https://www.zhihu.com/column/c_1375068546326716417)

[关于作业8的一些问题解答 – 计算机图形学与混合现实研讨会 (games-cn.org)](http://games-cn.org/forums/topic/guanyuzuoye8deyixiewentijieda/)

[games101 作业8 - coolwx - 博客园 (cnblogs.com)](https://www.cnblogs.com/coolwx/p/15059551.html)

[GAMES101 作业8 粒子间的弹簧模拟 - 哔哩哔哩 (bilibili.com)](https://www.bilibili.com/read/cv12440553)