import { redirect } from '@sveltejs/kit';

export function load({ url }) {
	const pb = url.searchParams.get('pb');
	const ps = url.searchParams.get('ps');

	const fileDataBinary = pb && Uint8Array.from(
		atob(pb),
		c => c.charCodeAt(0),
	);

	if (!fileDataBinary) redirect(307, '/upload');

	const fileDataSymbols = ps && Uint8Array.from(
		atob(ps),
		c => c.charCodeAt(0),
	);

	return { fileDataBinary, fileDataSymbols };
}
