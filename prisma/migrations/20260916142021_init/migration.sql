-- CreateTable
CREATE TABLE "measurements" (
    "id" SERIAL NOT NULL,
    "deviceId" TEXT NOT NULL,
    "temperature" DOUBLE PRECISION NOT NULL,
    "humidity" DOUBLE PRECISION NOT NULL,
    "recordedAt" TIMESTAMPTZ NOT NULL,
    "measuredAt" TIMESTAMPTZ,

    CONSTRAINT "measurements_pkey" PRIMARY KEY ("id")
);

-- CreateIndex
CREATE INDEX "measurements_deviceId_recordedAt_idx" ON "measurements"("deviceId", "recordedAt");
