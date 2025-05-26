### 第一种构造 (First Construction)

使用RLWE和RGSW加密实现二维折叠的测试文件：

1. **test-first-construction.cpp**
   - 测试第一种构造的核心实现
   - 使用`ksKey_hint_first_construction`和`answer_first_dimension`函数
   - 展示了完整的流程：查询生成、密钥生成、计算结果和解密

2. **test-first-dimension.cpp**
   - 测试第一种构造的一维部分
   - 使用`answer_first_dimension`函数
   - 也展示了密钥切换和响应计算过程

3. **test-two-dimensions.cpp**
   - 测试第一种构造的完整二维流程
   - 使用`answer_two_dimensions`函数
   - 结合了RLWE查询(第一维)和RGSW查询(第二维)

### 第二种构造 (Second Construction)

基于预处理优化的构造测试文件：

1. **test-setup.cpp**
   - 测试第二种构造的预处理阶段
   - 使用`publicKeyGen`、`evalKsKeyGen`和`modSwitch`函数
   - 演示了如何将计算量大的部分转移到离线阶段

2. **test-larger-database.cpp**
   - 测试在更大数据库上的第二种构造性能
   - 展示了预处理和在线阶段分开的流程

### 第三种构造 (Third Construction - BSGS算法)

使用"Baby-step-Giant-step"(BSGS)算法的测试文件：

1. **test-two-steps.cpp**
   - 包含多个测试函数，全面测试BSGS构造
   - `test_two_steps_bsgs`测试基本BSGS算法
   - `test_two_steps_bsgs_crt`测试使用中国剩余定理的BSGS
   - `test_two_steps_bsgs_rns`测试使用剩余数系统(RNS)的BSGS
   - `test_two_steps_bsgs_rns_large`测试大规模数据库的BSGS-RNS

2. **test-two-steps-key.cpp**
   - 测试BSGS算法中自动密钥生成
   - `test_two_steps_bsgs_rns`测试使用RNS的BSGS变体
   - `test_generate_autokey`测试从离线密钥生成在线密钥

3. **test-pir.cpp**
   - 最全面的测试，集成所有三种构造的要素
   - 特别关注第三种构造在大规模数据库上的性能
   - 测试大规模打包和BSGS优化

### 辅助功能测试

还有一些测试文件用于测试构造方法中的重要组件：

- **test-packing-lwes.cpp**：测试LWE和RLWE密文的打包技术
- **test-utils.cpp**：测试各种辅助功能，如分解、自同构变换和外积
- **test-expand.cpp**：测试向量扩展操作
- **test-ntt.cpp**和**test-ntt-speed.cpp**：测试NTT运算性能
- **test-multi.cpp**：测试大规模乘法操作
- **test-dummy-data.cpp**：测试使用虚拟数据的基本操作
