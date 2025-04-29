import { clsx } from 'clsx';
import { twMerge } from 'tailwind-merge';

export function twc(...inputs) {
	return twMerge(clsx(...inputs));
}
