-- AlterTable
ALTER TABLE "DeviceTelemetrySensors" RENAME CONSTRAINT "DeviceTelemetry_pkey" TO "DeviceTelemetrySensors_pkey";

-- RenameForeignKey
ALTER TABLE "DeviceTelemetrySensors" RENAME CONSTRAINT "DeviceTelemetry_deviceId_fkey" TO "DeviceTelemetrySensors_deviceId_fkey";

-- RenameIndex
ALTER INDEX "DeviceTelemetry_deviceId_created_idx" RENAME TO "DeviceTelemetrySensors_deviceId_created_idx";

-- RenameIndex
ALTER INDEX "DeviceTelemetry_deviceId_received_idx" RENAME TO "DeviceTelemetrySensors_deviceId_received_idx";

-- RenameIndex
ALTER INDEX "DeviceTelemetry_usage_created_idx" RENAME TO "DeviceTelemetrySensors_usage_created_idx";
