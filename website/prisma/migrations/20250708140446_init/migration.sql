-- CreateEnum
CREATE TYPE "BoardType" AS ENUM ('esp32s3_devkitc', 'frdm_rw612', 'nrf7002dk');

-- CreateEnum
CREATE TYPE "DeviceUsage" AS ENUM ('pot', 'keeper');

-- CreateEnum
CREATE TYPE "FirmwareFramework" AS ENUM ('zephyr', 'arduino', 'espidf');

-- CreateTable
CREATE TABLE "AvailableFirmware" (
    "id" TEXT NOT NULL,
    "created" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updated" TIMESTAMP(3) NOT NULL,
    "board" "BoardType" NOT NULL,
    "usage" "DeviceUsage" NOT NULL,
    "framework" "FirmwareFramework" NOT NULL,
    "versionValue" INTEGER NOT NULL,
    "versionSemver" TEXT NOT NULL,
    "url" TEXT NOT NULL,
    "attestation" TEXT NOT NULL,
    "hash" TEXT NOT NULL,
    "signature" TEXT NOT NULL,

    CONSTRAINT "AvailableFirmware_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "Device" (
    "id" TEXT NOT NULL,
    "created" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "updated" TIMESTAMP(3) NOT NULL,
    "board" "BoardType" NOT NULL,
    "usage" "DeviceUsage" NOT NULL,
    "framework" "FirmwareFramework",
    "label" VARCHAR(20) NOT NULL,
    "enrollmentId" TEXT NOT NULL,
    "certificatePem" TEXT,
    "provisioned" BOOLEAN NOT NULL DEFAULT false,
    "connected" BOOLEAN NOT NULL DEFAULT false,
    "lastSeen" TIMESTAMP(3),

    CONSTRAINT "Device_pkey" PRIMARY KEY ("id")
);

-- CreateTable
CREATE TABLE "DeviceActuator" (
    "deviceId" TEXT NOT NULL,
    "enabled" BOOLEAN NOT NULL DEFAULT false,
    "duration" INTEGER,
    "equilibriumTime" INTEGER,
    "target" INTEGER,
    "count" INTEGER,
    "lastTime" TIMESTAMP(3),
    "totalDuration" INTEGER,

    CONSTRAINT "DeviceActuator_pkey" PRIMARY KEY ("deviceId")
);

-- CreateTable
CREATE TABLE "DeviceFirmware" (
    "deviceId" TEXT NOT NULL,
    "currentVersion" TEXT,
    "desiredVersion" TEXT,
    "updateRequested" BOOLEAN NOT NULL DEFAULT false,
    "desiredFirmwareId" TEXT,

    CONSTRAINT "DeviceFirmware_pkey" PRIMARY KEY ("deviceId")
);

-- CreateTable
CREATE TABLE "DeviceTelemetry" (
    "id" TEXT NOT NULL,
    "deviceId" TEXT NOT NULL,
    "usage" "DeviceUsage" NOT NULL,
    "created" TIMESTAMP(3) NOT NULL,
    "received" TIMESTAMP(3),
    "temperature" DOUBLE PRECISION,
    "humidity" DOUBLE PRECISION,
    "moisture" DOUBLE PRECISION,
    "ph" DOUBLE PRECISION,

    CONSTRAINT "DeviceTelemetry_pkey" PRIMARY KEY ("id")
);

-- CreateIndex
CREATE UNIQUE INDEX "AvailableFirmware_board_usage_framework_versionSemver_key" ON "AvailableFirmware"("board", "usage", "framework", "versionSemver");

-- CreateIndex
CREATE INDEX "DeviceTelemetry_deviceId_created_idx" ON "DeviceTelemetry"("deviceId", "created");

-- CreateIndex
CREATE INDEX "DeviceTelemetry_deviceId_received_idx" ON "DeviceTelemetry"("deviceId", "received");

-- CreateIndex
CREATE INDEX "DeviceTelemetry_usage_created_idx" ON "DeviceTelemetry"("usage", "created");

-- AddForeignKey
ALTER TABLE "DeviceActuator" ADD CONSTRAINT "DeviceActuator_deviceId_fkey" FOREIGN KEY ("deviceId") REFERENCES "Device"("id") ON DELETE RESTRICT ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "DeviceFirmware" ADD CONSTRAINT "DeviceFirmware_deviceId_fkey" FOREIGN KEY ("deviceId") REFERENCES "Device"("id") ON DELETE RESTRICT ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "DeviceFirmware" ADD CONSTRAINT "DeviceFirmware_desiredFirmwareId_fkey" FOREIGN KEY ("desiredFirmwareId") REFERENCES "AvailableFirmware"("id") ON DELETE SET NULL ON UPDATE CASCADE;

-- AddForeignKey
ALTER TABLE "DeviceTelemetry" ADD CONSTRAINT "DeviceTelemetry_deviceId_fkey" FOREIGN KEY ("deviceId") REFERENCES "Device"("id") ON DELETE RESTRICT ON UPDATE CASCADE;
