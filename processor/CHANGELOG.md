# processor

## 0.4.0

### Minor Changes

- 864c742: Remove tinybird as we no longer need quick graphs

## 0.3.1

### Patch Changes

- e221991: Fix schema when sending actuators telemetry from processor to dashboard

## 0.3.0

### Minor Changes

- 447df11: Add support for storing actuation records once they are produced

## 0.2.2

### Patch Changes

- f15138b: Processor should not have default blob storage endpoint to allow connection string override

## 0.2.1

### Patch Changes

- 96dc6f9: Allow use of connection string in processor
- 367cb3d: Reduce replica timeout from 10 minutes to 5 minutes
- e4cd17f: Fix env variable name for hub name
- bdc0a42: Do not specify EntityPath in connection string

## 0.2.0

### Minor Changes

- 0c68152: No longer allow sending to the dashboard to fail
- 199b01f: Send telemetry to tinybird in hopes of quick graphs

### Patch Changes

- 81f49a8: Fix communication between processor and website

## 0.1.0

### Minor Changes

- 860a393: Add processor for IoT hub events
