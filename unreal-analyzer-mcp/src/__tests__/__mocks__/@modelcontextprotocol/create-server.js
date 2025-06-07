import { jest } from '@jest/globals';

const mockServer = {
  setRequestHandler: jest.fn(),
  connect: jest.fn(),
  close: jest.fn(),
  onerror: jest.fn()
};

export const Server = jest.fn(() => mockServer);
export const StdioServerTransport = jest.fn();
