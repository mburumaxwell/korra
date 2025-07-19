-- CreateTable
CREATE TABLE "DeviceTelemetryActuation" (
    "id" TEXT NOT NULL,
    "deviceId" TEXT NOT NULL,
    "usage" "DeviceUsage" NOT NULL,
    "created" TIMESTAMP(3) NOT NULL,
    "received" TIMESTAMP(3),
    "fan" INTEGER,
    "pump" INTEGER,

    CONSTRAINT "DeviceTelemetryActuation_pkey" PRIMARY KEY ("id")
);

-- CreateIndex
CREATE INDEX "DeviceTelemetryActuation_deviceId_created_idx" ON "DeviceTelemetryActuation"("deviceId", "created");

-- CreateIndex
CREATE INDEX "DeviceTelemetryActuation_deviceId_received_idx" ON "DeviceTelemetryActuation"("deviceId", "received");

-- CreateIndex
CREATE INDEX "DeviceTelemetryActuation_usage_created_idx" ON "DeviceTelemetryActuation"("usage", "created");

-- AddForeignKey
ALTER TABLE "DeviceTelemetryActuation" ADD CONSTRAINT "DeviceTelemetryActuation_deviceId_fkey" FOREIGN KEY ("deviceId") REFERENCES "Device"("id") ON DELETE RESTRICT ON UPDATE CASCADE;
