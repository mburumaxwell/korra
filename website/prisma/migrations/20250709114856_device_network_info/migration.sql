-- CreateEnum
CREATE TYPE "NetworkKind" AS ENUM ('wifi', 'ethernet', 'cellular');

-- CreateTable
CREATE TABLE "DeviceNetwork" (
    "deviceId" TEXT NOT NULL,
    "kind" "NetworkKind",
    "mac" VARCHAR(50),
    "name" VARCHAR(50),
    "local_ip" VARCHAR(100),

    CONSTRAINT "DeviceNetwork_pkey" PRIMARY KEY ("deviceId")
);

-- AddForeignKey
ALTER TABLE "DeviceNetwork" ADD CONSTRAINT "DeviceNetwork_deviceId_fkey" FOREIGN KEY ("deviceId") REFERENCES "Device"("id") ON DELETE RESTRICT ON UPDATE CASCADE;
