-- CreateEnum
CREATE TYPE "MoistureCategory" AS ENUM ('dry', 'medium', 'wet');

-- AlterTable
ALTER TABLE "Device" ADD COLUMN     "moistureCategory" "MoistureCategory";
