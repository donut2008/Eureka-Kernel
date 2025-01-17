PONSE_INTERRUPT_COUNT_MASK 0xff
#define RESPONSE_INTERRUPT_COUNT__N_RESPONSE_INTERRUPT_COUNT__SHIFT 0x0
#define RIRB_CONTROL__RESPONSE_INTERRUPT_CONTROL_MASK 0x1
#define RIRB_CONTROL__RESPONSE_INTERRUPT_CONTROL__SHIFT 0x0
#define RIRB_CONTROL__RIRB_DMA_ENABLE_MASK 0x2
#define RIRB_CONTROL__RIRB_DMA_ENABLE__SHIFT 0x1
#define RIRB_CONTROL__RESPONSE_OVERRUN_INTERRUPT_CONTROL_MASK 0x4
#define RIRB_CONTROL__RESPONSE_OVERRUN_INTERRUPT_CONTROL__SHIFT 0x2
#define RIRB_STATUS__RESPONSE_INTERRUPT_MASK 0x1
#define RIRB_STATUS__RESPONSE_INTERRUPT__SHIFT 0x0
#define RIRB_STATUS__RESPONSE_OVERRUN_INTERRUPT_STATUS_MASK 0x4
#define RIRB_STATUS__RESPONSE_OVERRUN_INTERRUPT_STATUS__SHIFT 0x2
#define RIRB_SIZE__RIRB_SIZE_MASK 0x3
#define RIRB_SIZE__RIRB_SIZE__SHIFT 0x0
#define RIRB_SIZE__RIRB_SIZE_CAPABILITY_MASK 0xf0
#define RIRB_SIZE__RIRB_SIZE_CAPABILITY__SHIFT 0x4
#define IMMEDIATE_COMMAND_OUTPUT_INTERFACE__IMMEDIATE_COMMAND_WRITE_VERB_AND_PAYLOAD_MASK 0xfffffff
#define IMMEDIATE_COMMAND_OUTPUT_INTERFACE__IMMEDIATE_COMMAND_WRITE_VERB_AND_PAYLOAD__SHIFT 0x0
#define IMMEDIATE_COMMAND_OUTPUT_INTERFACE__IMMEDIATE_COMMAND_WRITE_CODEC_ADDRESS_MASK 0xf0000000
#define IMMEDIATE_COMMAND_OUTPUT_INTERFACE__IMMEDIATE_COMMAND_WRITE_CODEC_ADDRESS__SHIFT 0x1c
#define IMMEDIATE_COMMAND_OUTPUT_INTERFACE_INDEX__IMMEDIATE_COMMAND_WRITE_MASK 0xffff
#define IMMEDIATE_COMMAND_OUTPUT_INTERFACE_INDEX__IMMEDIATE_COMMAND_WRITE__SHIFT 0x0
#define IMMEDIATE_COMMAND_OUTPUT_INTERFACE_DATA__IMMEDIATE_COMMAND_WRITE_MASK 0xffffffff
#define IMMEDIATE_COMMAND_OUTPUT_INTERFACE_DATA__IMMEDIATE_COMMAND_WRITE__SHIFT 0x0
#define IMMEDIATE_RESPONSE_INPUT_INTERFACE__IMMEDIATE_RESPONSE_READ_MASK 0xffffffff
#define IMMEDIATE_RESPONSE_INPUT_INTERFACE__IMMEDIATE_RESPONSE_READ__SHIFT 0x0
#define IMMEDIATE_COMMAND_STATUS__IMMEDIATE_COMMAND_BUSY_MASK 0x1
#define IMMEDIATE_COMMAND_STATUS__IMMEDIATE_COMMAND_BUSY__SHIFT 0x0
#define IMMEDIATE_COMMAND_STATUS__IMMEDIATE_RESULT_VALID_MASK 0x2
#define IMMEDIATE_COMMAND_STATUS__IMMEDIATE_RESULT_VALID__SHIFT 0x1
#define DMA_POSITION_LOWER_BASE_ADDRESS__DMA_POSITION_BUFFER_ENABLE_MASK 0x1
#define DMA_POSITION_LOWER_BASE_ADDRESS__DMA_POSITION_BUFFER_ENABLE__SHIFT 0x0
#define DMA_POSITION_LOWER_BASE_ADDRESS__DMA_POSITION_LOWER_BASE_UNIMPLEMENTED_BITS_MASK 0x7e
#define DMA_POSITION_LOWER_BASE_ADDRESS__DMA_POSITION_LOWER_BASE_UNIMPLEMENTED_BITS__SHIFT 0x1
#define DMA_POSITION_LOWER_BASE_ADDRESS__DMA_POSITION_LOWER_BASE_ADDRESS_MASK 0xffffff80
#define DMA_POSITION_LOWER_BASE_ADDRESS__DMA_POSITION_LOWER_BASE_ADDRESS__SHIFT 0x7
#define DMA_POSITION_UPPER_BASE_ADDRESS__DMA_POSITION_UPPER_BASE_ADDRESS_MASK 0xffffffff
#define DMA_POSITION_UPPER_BASE_ADDRESS__DMA_POSITION_UPPER_BASE_ADDRESS__SHIFT 0x0
#define WALL_CLOCK_COUNTER_ALIAS__WALL_CLOCK_COUNTER_ALIAS_MASK 0xffffffff
#define WALL_CLOCK_COUNTER_ALIAS__WALL_CLOCK_COUNTER_ALIAS__SHIFT 0x0
#define OUTPUT_STREAM_DESCRIPTOR_CONTROL_AND_STATUS__STREAM_RESET_MASK 0x1
#define OUTPUT_STREAM_DESCRIPTOR_CONTROL_AND_STATUS__STREAM_RESET__SHIFT 0x0
#define OUTPUT_STREAM_DESCRIPTOR_CONTROL_AND_STATUS__STREAM_RUN_MASK 0x2
#define OUTPUT_STREAM_DESCRIPTOR_CONTROL_AND_STATUS__STREAM_RUN__SHIFT 0x1
#define OUTPUT_STREAM_DESCRIPTOR_CONTROL_AND_STATUS__INTERRUPT_ON_COMPLETION_ENABLE_MASK 0x4
#define OUTPUT_STREAM_DESCRIPTOR_CONTROL_AND_STATUS__INTERRUPT_ON_COMPLETION_ENABLE__SHIFT 0x2
#define OUTPUT_STREAM_DESCRIPTOR_CONTROL_AND_STATUS__FIFO_ERROR_INTERRUPT_ENABLE_MASK 0x8
#define OUTPUT_STREAM_DESCRIPTOR_CONTROL_AND_STATUS__FIFO_ERROR_INTERRUPT_ENABLE__SHIFT 0x3
#define OUTPUT_STREAM_DESCRIPTOR_CONTROL