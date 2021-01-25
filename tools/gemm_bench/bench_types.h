


// Backend Engine
enum EngineTypeEnum {
    eINVALID = -1,
    eONEDNN = 1,
    eMKL = 2,
    eEIGEN = 3,
};

template <EngineTypeEnum T>
struct EngineType {};

typedef EngineType<eONEDNN> ONEDNN;
typedef EngineType<eMKL> MKL;
typedef EngineType<eEIGEN> EIGEN;


// Data Types
enum DataType {
    INVALID = -1,
    FP16    = 0,
    FP32    = 1,
    FP64    = 2,
    INT8    = 3,
    INT16   = 4,
    INT32   = 5,
    INT64   = 6,
    UINT8   = 7,
    UINT16  = 8,
    UINT32  = 9,
    STRING  = 10,
    BOOL    = 11,
};


// Activation Types
typedef enum{
    Activation_none = 0,
    Activation_sigmoid = 1,
    Activation_relu = 2,
    Activation_tanh = 3,
    Activation_clipped_relu = 4,
    Activation_elu = 5,
    Activation_identity = 6,
    Activation_sigmoid_fluid = 7,
    Activation_tanh_fluid = 8,
    Activation_stanh = 9,
    Activation_prelu = 10
} ActivationType;

