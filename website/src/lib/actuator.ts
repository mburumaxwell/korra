import { type MoistureCategory } from './prisma/client';

export const ACTUATOR_TARGETS_POT: Record<MoistureCategory, number> = {
  dry: 25,
  medium: 45,
  wet: 65,
};
